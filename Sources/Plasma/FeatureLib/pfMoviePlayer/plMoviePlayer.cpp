/*==LICENSE==*

CyanWorlds.com Engine - MMOG client, server and tools
Copyright (C) 2011  Cyan Worlds, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Additional permissions under GNU GPL version 3 section 7

If you modify this Program, or any covered work, by linking or
combining it with any of RAD Game Tools Bink SDK, Autodesk 3ds Max SDK,
NVIDIA PhysX SDK, Microsoft DirectX SDK, OpenSSL library, Independent
JPEG Group JPEG library, Microsoft Windows Media SDK, or Apple QuickTime SDK
(or a modified version of those libraries),
containing parts covered by the terms of the Bink SDK EULA, 3ds Max EULA,
PhysX SDK EULA, DirectX SDK EULA, OpenSSL and SSLeay licenses, IJG
JPEG Library README, Windows Media SDK EULA, or QuickTime SDK EULA, the
licensors of this Program grant you additional
permission to convey the resulting work. Corresponding Source for a
non-source form of such a combination shall include the source code for
the parts of OpenSSL and IJG JPEG Library used as well as that of the covered
work.

You can contact Cyan Worlds, Inc. by email legal@cyan.com
 or by snail mail at:
      Cyan Worlds, Inc.
      14617 N Newport Hwy
      Mead, WA   99021

*==LICENSE==*/

#include "plMoviePlayer.h"

#include <tuple>

#ifdef VPX_AVAILABLE
#   define VPX_CODEC_DISABLE_COMPAT 1
#   include <vpx/vpx_decoder.h>
#   include <vpx/vp8dx.h>
#   define iface (vpx_codec_vp8_dx())
#endif

#include "plFile/plFileUtils.h"
#include "plGImage/plMipmap.h"
#include "pnKeyedObject/plUoid.h"
#include "plPipeline/hsGDeviceRef.h"
#include "plPipeline/plPlates.h"
#include "hsResMgr.h"
#include "hsTimer.h"

#include "webm/mkvreader.hpp"
#include "webm/mkvparser.hpp"

#define SAFE_OP(x, err) \
    { \
        int64_t ret = 0; \
        ret = x; \
        if (ret == -1) \
        { \
            hsAssert(false, "failed to " err); \
            return false; \
        } \
    }

// =====================================================

class VPX
{
    VPX() { }

public:
    vpx_codec_ctx_t codec;

    ~VPX() {
        if (vpx_codec_destroy(&codec))
            hsAssert(false, vpx_codec_error_detail(&codec));
    }

    static VPX* Create()
    {
        VPX* instance = new VPX;
        if(vpx_codec_dec_init(&instance->codec, iface, nullptr, 0))
        {
            hsAssert(false, vpx_codec_error_detail(&instance->codec));
            return nullptr;
        }
        return instance;
    }

    vpx_image_t* Decode(uint8_t* buf, uint32_t size)
    {
        if (vpx_codec_decode(&codec, buf, size, nullptr, 0))
        {
            hsAssert(false, vpx_codec_error_detail(&codec));
            return nullptr;
        }

        vpx_codec_iter_t  iter = nullptr;
        // ASSUMPTION: only one image per frame
        // if this proves false, move decoder function into IProcessVideoFrame
        return vpx_codec_get_frame(&codec, &iter);
    }
};

// =====================================================

class TrackMgr
{
    const mkvparser::BlockEntry* blk_entry;
    bool valid;

    bool PeekNextBlockEntry(const std::unique_ptr<mkvparser::Segment>& segment)
    {
        // Assume that if blk_entry == nullptr, we need to start from the beginning
        const mkvparser::Cluster* cluster;
        if (blk_entry)
            cluster = blk_entry->GetCluster();
        else
            cluster = segment->GetFirst();

        while (true)
        {
            if (cluster->EOS())
            {
                cluster = segment->GetNext(cluster);
                blk_entry = nullptr;
                if (!cluster)
                    return false;
            }
            if (blk_entry)
            {
                SAFE_OP(cluster->GetNext(blk_entry, blk_entry), "get next block");
            }
            else
            {
                SAFE_OP(cluster->GetFirst(blk_entry), "get first block");
            }

            if (blk_entry)
            {
                if (blk_entry->EOS())
                    continue;
                if (blk_entry->GetBlock()->GetTrackNumber() == number)
                    return true;
            }
        }
        return false; // if this happens, boom.
    }

public:
    uint32_t number;

    TrackMgr(uint32_t num) : blk_entry(nullptr), valid(true), number(num) { }

    bool Advance(plMoviePlayer* p, int64_t movieTime)
    {
        if (!valid)
            return false;

        // This keeps us from getting behind due to freezes
        // Assumption: Audio will not skip ahead in time. FIXME?
        while ((valid = PeekNextBlockEntry(p->fSegment)))
        {
            const mkvparser::Block* blk = blk_entry->GetBlock();
            if (blk->GetTime(blk_entry->GetCluster()) < movieTime)
                continue;
            else
                return true;
        }
        return false; // ran out of blocks
    }

    int32_t GetBlockData(plMoviePlayer* p, std::vector<blkbuf_t>& frames) const
    {
        const mkvparser::Block* block = blk_entry->GetBlock();

        // Return the frames
        frames.reserve(block->GetFrameCount());
        for (int32_t i = 0; i < block->GetFrameCount(); ++i)
        {
            const mkvparser::Block::Frame frame = block->GetFrame(i);
            std::shared_ptr<uint8_t> data(new uint8_t[frame.len]);
            frame.Read(p->fReader, data.get());
            frames.push_back(std::make_tuple(data, frame.len));
        }
        return block->GetFrameCount();
    }
};

// =====================================================

plMoviePlayer::plMoviePlayer() :
    fPlate(nullptr),
    fTexture(nullptr),
    fReader(nullptr),
    fTimeScale(0), 
    fStartTime(0)
{
    fScale.Set(1.0f, 1.0f);
}

plMoviePlayer::~plMoviePlayer()
{
    if (fPlate)
        // The plPlate owns the Mipmap Texture, so it destroys it for us
        plPlateManager::Instance().DestroyPlate(fPlate);
#ifdef VPX_AVAILABLE
    if (fReader)
        fReader->Close();
#endif
}

int64_t plMoviePlayer::GetMovieTime() const
{
    return ((int64_t)hsTimer::GetSeconds() * fTimeScale) - fStartTime;
}

bool plMoviePlayer::IOpenMovie()
{
#ifdef VPX_AVAILABLE
    if (fMoviePath.IsEmpty())
    {
        hsAssert(false, "Tried to play an empty movie");
        return false;
    }
    if (!plFileUtils::FileExists(fMoviePath.c_str()))
    {
        hsAssert(false, "Tried to play a movie that doesn't exist");
        return false;
    }

    // open the movie with libwebm
    fReader = new mkvparser::MkvReader;
    SAFE_OP(fReader->Open(fMoviePath.c_str()), "open movie");

    // opens the segment
    // it contains everything you ever want to know about the movie
    long long pos = 0;
    mkvparser::EBMLHeader ebmlHeader;
    ebmlHeader.Parse(fReader, pos);
    mkvparser::Segment* seg;
    SAFE_OP(mkvparser::Segment::CreateInstance(fReader, pos, seg), "get segment info");
    SAFE_OP(seg->Load(), "load segment from webm");
    fSegment.reset(seg);

    // Just in case someone gives us a weird file, find out the timecode offset
    fTimeScale = fSegment->GetInfo()->GetTimeCodeScale();

    // TODO: Figure out video and audio based on current language
    //       For now... just take the first one.
    const mkvparser::Tracks* tracks = fSegment->GetTracks();
    for (uint32_t i = 0; i < tracks->GetTracksCount(); ++i)
    {
        const mkvparser::Track* track = tracks->GetTrackByIndex(i);
        if (!track)
            continue;

        switch (track->GetType())
        {
        case mkvparser::Track::kAudio:
            {
                if (!fAudioTrack)
                    fAudioTrack.reset(new TrackMgr(track->GetNumber()));
                break;
            }
        case mkvparser::Track::kVideo:
            {
                if (!fVideoTrack)
                    fVideoTrack.reset(new TrackMgr(track->GetNumber()));
                break;
            }
        }
    }
    return true;
#else
    return false;
#endif
}

static uint8_t Clip(int32_t val) {
    if (val < 0) {
        return 0;
    } else if (val > 255) {
        return 255;
    }
    return static_cast<uint8_t>(val);
}

#define YG 74 /* static_cast<int8>(1.164 * 64 + 0.5) */

#define UB 127 /* min(63,static_cast<int8>(2.018 * 64)) */
#define UG -25 /* static_cast<int8>(-0.391 * 64 - 0.5) */
#define UR 0

#define VB 0
#define VG -52 /* static_cast<int8>(-0.813 * 64 - 0.5) */
#define VR 102 /* static_cast<int8>(1.596 * 64 + 0.5) */

// Bias
#define BB UB * 128 + VB * 128
#define BG UG * 128 + VG * 128
#define BR UR * 128 + VR * 128

bool plMoviePlayer::IProcessVideoFrame(const blkbuf_t& data)
{
    uint8_t* buf = std::get<0>(data).get();
    int32_t size = std::get<1>(data);
    vpx_image_t* img = fVpx->Decode(buf, static_cast<uint32_t>(size));
    if (img)
    {
        // gigantic fixme. This currently contains a bunch of stuff that should be
        // abstracted, and only works for planar YUV420 data.
        uint8_t* dst = static_cast<uint8_t*>(fTexture->GetImage());
        uint8_t* y_src = static_cast<uint8_t*>(img->planes[0]);
        uint8_t* u_src = static_cast<uint8_t*>(img->planes[1]);
        uint8_t* v_src = static_cast<uint8_t*>(img->planes[2]);
        for(size_t i = 0; i < img->d_h; ++i) {
            for(size_t j = 0; j < img->d_w; ++j) {
                size_t y_idx = img->stride[0]*i+j;
                size_t u_idx = img->stride[1]*(i/2)+(j/2);
                size_t v_idx = img->stride[2]*(i/2)+(j/2);
                size_t dst_idx = img->d_w*i+j;
                int32_t y = static_cast<int32_t>(y_src[y_idx]);
                int32_t u = static_cast<int32_t>(u_src[u_idx]);
                int32_t v = static_cast<int32_t>(v_src[v_idx]);
                int32_t y1 = (y - 16) * YG;
                dst[dst_idx*4+0] = Clip(((u * UB + v * VB) - (BB) + y1) >> 6);
                dst[dst_idx*4+1] = Clip(((u * UG + v * VG) - (BG) + y1) >> 6);
                dst[dst_idx*4+2] = Clip(((u * UR + v * VR) - (BR) + y1) >> 6);
                dst[dst_idx*4+3] = 0xff;
            }
        }
        //memcpy(ptr, img->planes[0], (img->d_w * img->d_h * 1));
        //fTexture->SetImagePtr(img->img_data); // hack
        if (fTexture->GetDeviceRef())
            fTexture->GetDeviceRef()->SetDirty(true);
        return true;
    }
    return false;
}

bool plMoviePlayer::Start()
{
#ifdef VPX_AVAILABLE
    if (!IOpenMovie())
        return false;
    hsAssert(fVideoTrack, "nil video track -- expect bad things to happen!");

    // Initialize VP8
    if (VPX* vpx = VPX::Create())
        fVpx.reset(vpx);
    else
        return false;

    // Need to figure out scaling based on pipe size.
    plPlateManager& plateMgr = plPlateManager::Instance();
    const mkvparser::VideoTrack* video = static_cast<const mkvparser::VideoTrack*>(fSegment->GetTracks()->GetTrackByNumber(fVideoTrack->number));
    float width = (float)plateMgr.GetPipeWidth() / ((float)video->GetWidth() * fScale.fX);
    float height = (float)plateMgr.GetPipeHeight() / ((float)video->GetHeight() * fScale.fY);

    plateMgr.CreatePlate(&fPlate, fPosition.fX, fPosition.fY, width, height);
    fPlate->SetVisible(true);
    uint32_t w = static_cast<uint32_t>(video->GetWidth());
    uint32_t h = static_cast<uint32_t>(video->GetHeight());
    fTexture = fPlate->CreateMaterial(w, h, false);
    return true;
#else
    return false;
#endif // VPX_AVAILABLE
}

bool plMoviePlayer::NextFrame()
{
#ifdef VPX_AVAILABLE
    // Get our current timecode
    int64_t movieTime = 0;
    if (fStartTime == 0)
        fStartTime = (int64_t)(hsTimer::GetSeconds() * fTimeScale);
    else
        movieTime = GetMovieTime();

    // Check to see if we're at the end of the movie... Also advances webm stuff
    // TODO: tracks vector
    uint8_t tracksWithData = 0;
    if (fAudioTrack)
    {
        if (fAudioTrack->Advance(this, movieTime))
            tracksWithData++;
    }
    if (fVideoTrack)
    {
        if (fVideoTrack->Advance(this, movieTime))
            tracksWithData++;
    }
    if (tracksWithData == 0)
        return false;

    std::vector<blkbuf_t> frames;
    fVideoTrack->GetBlockData(this, frames);
    hsAssert(frames.size() == 1, "only support one video frame per block");
    if (frames.size())
        IProcessVideoFrame(frames[0]);
    frames.clear(); // :D

    // TODO: audio
    return true;
#else
    return false;
#endif // VPX_AVAILABLE
}
