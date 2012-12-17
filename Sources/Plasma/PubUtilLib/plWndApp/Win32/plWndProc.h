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

#ifndef _plWndProc__INC
#define _plWndProc__INC

#include <memory>

#include "HeadSpin.h"
#include "plString.h"
#include "hsWindows.h"

class plControl;
class plWindow;

class plWndProc
{
public:
    enum FontStyle { kNormalCaption, kSmallCaption, kMenu, kStatus, kMessage };

    HWND fHWND;
    std::shared_ptr<HFONT__> fFont;

    ~plWndProc() { }

    bool SetFont(FontStyle style, const plString& face=plString::Null);
    void SetFont(const std::shared_ptr<HFONT__>& font);

    plWndProc() : fHWND(0) { }
    virtual LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
};

class plWndImp : public plWndProc
{
    plWindow* fOwner;
    static bool fCreatedWndClass;

public:
    plWndImp(plWindow* p) : fOwner(p) { }

    bool Create(const plStringBuffer<wchar_t>& title, uint32_t width, uint32_t height, HWND parent);

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

class plCtrlProc : public plWndProc
{
protected:
    plControl* fOwner;
public:
    plCtrlProc(plControl* owner) : fOwner(owner), plWndProc() { }

    bool Create(const wchar_t* cls, uint32_t style=0);
    bool InitCommCtrl(uint32_t type) const;
    void InheritFont();
};

class plStaticCtrlProc : public plCtrlProc
{
public:
    plStaticCtrlProc(plControl* owner) : plCtrlProc(owner) { }

    LONG_PTR Color(HDC hdc);
};

#endif // _plWndProc__INC
