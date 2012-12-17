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

#ifndef _plApp__INC
#define _plApp__INC

#include "plString.h"
#include <unordered_map>

/**
 * A lightweight argument parser.
 *
 * [detail]
 */
class plArgParser
{
    // FIXME: TEMPORARY -- remove when plString has a real hash function
    struct StringHash : public std::hash<const char*>
    {
        size_t operator()(const plString& value) const
        {
            return std::hash<const char*>::operator()(value.c_str());
        }
    };

    std::unordered_map<plString, plString, StringHash> fThings;

    void ProcessArgument(const plString& arg, std::pair<plString, plString>& curr);

public:
    plArgParser(const char** argv, int argc);
#ifdef HS_BUILD_FOR_WIN32
    plArgParser(const wchar_t* cmdLine);
#endif

    template<bool>
    bool Get(const plString& key, bool def=false) const
    {
        auto it = fThings.find(key);
        if (it == fThings.end())
            return def;
        else
            return ((*it).second.ToInt() == 1);
    }

    template<int32_t>
    int32_t Get(const plString& key, int32_t def=0) const
    {
        auto it = fThings.find(key);
        if (it == fThings.end())
            return def;
        else
            return (*it).second.ToInt();
    }

    template<class plString>
    plString Get(const plString& key, const plString& def="") const
    {
        auto it = fThings.find(key);
        if (it == fThings.end())
            return def;
        else
            return (*it).second;
    }
};

namespace plApp
{
    void MainLoop()
    {
        MSG msg;
        while (GetMessageW(&msg, 0, 0, 0))
        {
            if (msg.message == WM_QUIT)
                break;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void Quit()
    {
#ifdef HS_BUILD_FOR_WIN32
        PostQuitMessage(0);
#endif
    }
};

#ifdef HS_BUILD_FOR_WIN32
    HINSTANCE g_hInstance;

#   define APP_MAIN_BEGIN \
    int WINAPI wWinMain(HINSTANCE _hinst, HINSTANCE, wchar_t* _cmdLine, int _cmdShow) \
    { \
        g_hInstance = _hinst;
#   define APP_PARSE_ARGS plArgParser args(_cmdLine);
#   define APP_MAIN_END }

#else
#   define APP_MAIN_BEGIN int main(char** argv, int argc) {
#   define APP_PARSE_ARGS plArgParser args(argv, argc);
#   define APP_MAIN_END }
#endif

#define APP_FAILURE 1
#define APP_SUCCESS 0

#endif // _plApp__INC
