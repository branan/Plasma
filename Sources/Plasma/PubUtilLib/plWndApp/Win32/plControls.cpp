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

#include "plWndProc.h"
#include <commctrl.h>
#include <shlwapi.h>
#pragma hdrstop

#include "plControls.h"
#include "plWindow.h"

// Ensures we pull in Common Controls 6 (look decent on WinXP+)
#pragma comment(linker, "\"/manifestdependency:type='win32' \
                        name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
                        processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

extern HINSTANCE g_hInstance;

bool plCtrlProc::Create(const wchar_t* cls, uint32_t style)
{
    fHWND = CreateWindowW(cls, 0, style | WS_CHILD | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, fOwner->fParent->fImp->fHWND, 0, g_hInstance, 0);
    if (fHWND)
    {
        SetWindowLongPtrW(fHWND, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        SetWindowPos(fHWND, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        return true;
    }
    return false;
}

bool plCtrlProc::InitCommCtrl(uint32_t type) const
{
    INITCOMMONCONTROLSEX init;
    memset(&init, 0, sizeof(INITCOMMONCONTROLSEX));
    init.dwSize = sizeof(INITCOMMONCONTROLSEX);
    init.dwICC = type;
    return (InitCommonControlsEx(&init) != FALSE);
}

void plCtrlProc::InheritFont()
{
    SetFont(fOwner->fParent->fImp->fFont);
}

/////////////////////////////////////////////////////////////////////////////

bool plControl::Resize(int32_t width, int32_t height)
{
    return (SetWindowPos(fImp->fHWND, 0, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER) != FALSE);
}

bool plControl::SetPosition(int32_t x, int32_t y)
{
    return (SetWindowPos(fImp->fHWND, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER) != FALSE);
}

/////////////////////////////////////////////////////////////////////////////

plString plTextControl::GetText() const
{
    int size = GetWindowTextLengthW(fImp->fHWND);
    if (size > 0)
    {
        plStringBuffer<wchar_t> str;
        wchar_t* buf = str.CreateWritableBuffer(size);
        GetWindowTextW(fImp->fHWND, buf, size);
        buf[size] = 0; // you never know with win32
        return plString::FromWchar(str);
    }
    return "";
}

bool plTextControl::SetText(const plString& text)
{
    plStringBuffer<wchar_t> buf = text.ToWchar();
    return (SetWindowTextW(fImp->fHWND, buf.GetData()) != FALSE);
}

/////////////////////////////////////////////////////////////////////////////

plLabelControl::plLabelControl(plWindow* parent)
    : plTextControl(parent, new plStaticCtrlProc(this))
{
    hsAssert(fImp->InitCommCtrl(ICC_STANDARD_CLASSES), "failed to init Standard Classes");
    hsAssert(fImp->Create(WC_STATICW), "failed to create WC_STATICW");
    fImp->InheritFont();
}

plLabelControl::~plLabelControl()
{
}

/////////////////////////////////////////////////////////////////////////////

LONG_PTR plStaticCtrlProc::Color(HDC hdc)
{
    SetBkMode(hdc, COLOR_WINDOW);
    return reinterpret_cast<LONG_PTR>(GetSysColorBrush(COLOR_WINDOW));
}

/////////////////////////////////////////////////////////////////////////////

plTextBoxControl::plTextBoxControl(plWindow* parent, bool isPassword)
    : plTextControl(parent)
{
    uint32_t style = WS_BORDER | ES_AUTOHSCROLL | ES_LEFT;
    if (isPassword)
        style |= ES_PASSWORD;

    hsAssert(fImp->InitCommCtrl(ICC_STANDARD_CLASSES), "failed to init Standard Classes");
    hsAssert(fImp->Create(WC_EDITW, style), "failed to create WC_EDITW");
    fImp->InheritFont();
}

plTextBoxControl::~plTextBoxControl()
{
}
