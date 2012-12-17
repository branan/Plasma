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
#pragma hdrstop

#include "plControls.h"
#include "plWindow.h"

extern HINSTANCE g_hInstance;

static void _GdiObjectDeleter(HGDIOBJ ptr)
{
    hsAssert(DeleteObject(ptr), "failed to delete GDI object");
}

static plWndProc* _GetImpFromCreateStruct(LPARAM lParam)
{
    if (lParam)
    {
        CREATESTRUCTW* csw = reinterpret_cast<CREATESTRUCTW*>(lParam);
        if (csw->lpCreateParams)
            return static_cast<plWndProc*>(csw->lpCreateParams);
    }
    return nullptr;
}

LRESULT CALLBACK WndProcRedirect(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LONG_PTR wndImpl = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (wndImpl)
    {
        plWndProc* imp = reinterpret_cast<plWndProc*>(wndImpl);
        return imp->WndProc(hwnd, msg, wParam, lParam);
    }
    else
    {
        // messages that get sent before we setup the GWLP_USERDATA
        switch (msg)
        {
        case WM_CREATE:
        case WM_NCCREATE:
            plWndProc* imp = _GetImpFromCreateStruct(lParam);
            hsAssert(imp, "nil imp in CREATESTRUCTW");
            return imp->WndProc(hwnd, msg, wParam, lParam);
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////

bool plWndProc::SetFont(FontStyle style, const plString& face)
{
    NONCLIENTMETRICSW metrics;
    metrics.cbSize = sizeof(NONCLIENTMETRICSW);
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics, 0) == FALSE)
    {
        hsAssert(false, "failed to get nonclient metrics");
        return false;
    }

    LOGFONTW theFont;
    switch (style)
    {
    case kNormalCaption:
        theFont = metrics.lfCaptionFont;
        break;
    case kSmallCaption:
        theFont = metrics.lfSmCaptionFont;
        break;
    case kMenu:
        theFont = metrics.lfMenuFont;
        break;
    case kStatus:
        theFont = metrics.lfStatusFont;
        break;
    case kMessage:
        theFont = metrics.lfMessageFont;
        break;
    }

    // allow overrides of the font face, just for the heck of it.
    if (!face.IsEmpty())
    {
        const wchar_t* name = face.ToWchar().GetData();
        wcsncpy(theFont.lfFaceName, name, arrsize(theFont.lfFaceName));
    }

    // create our HFONT
    HFONT font = CreateFontIndirectW(&theFont);
    if (font)
    {
        SetFont(std::shared_ptr<HFONT__>(font, _GdiObjectDeleter));
        return true;
    }
   return false;
}

void plWndProc::SetFont(const std::shared_ptr<HFONT__>& font)
{
    if (font)
    {
        fFont = font; // whee copy
        SendMessageW(fHWND, WM_SETFONT, reinterpret_cast<WPARAM>(font.get()), TRUE);
    }
    else
        fFont.reset();
}

bool plWndImp::fCreatedWndClass = false;

bool plWndImp::Create(const plStringBuffer<wchar_t>& title, uint32_t width, uint32_t height, HWND parent)
{
    if (!fCreatedWndClass)
    {
        WNDCLASSW wndcls;
        memset(&wndcls, 0, sizeof(WNDCLASSW));
        wndcls.lpszClassName = L"Plasma20 WndAppCls";
        wndcls.hCursor = LoadCursorA(0, IDC_ARROW);
        wndcls.hInstance = g_hInstance;
        wndcls.lpszMenuName = 0;
        wndcls.lpfnWndProc = WndProcRedirect;
        wndcls.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
        RegisterClassW(&wndcls);
        fCreatedWndClass = true;
    }

    fHWND = CreateWindowW(L"Plasma20 WndAppCls", title.GetData(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        width, height, parent, 0, g_hInstance, this);
    if (fHWND)
    {
        SetWindowLongPtrW(fHWND, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        // needed as of NT6...
        SetWindowPos(fHWND, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        return true;
    }
    return false;
}

LRESULT CALLBACK plWndImp::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        if (fOwner->onRequestClose())
            hsAssert(DestroyWindow(fHWND), "failed to destroy hwnd");
        return 0;
    case WM_CREATE:
        // Set a default font for all our chilrens
        SetFont(plWndProc::kMessage);
        // allow the subclass to setup its controls
        fOwner->layout();
        return 0;
    case WM_NCCREATE:
        fHWND = hwnd; // hack?
        return static_cast<BOOL>(fOwner->onCreate());
    case WM_CTLCOLORSTATIC:
        {
            // Pass off a static control color message to its control proc
            LONG_PTR data = GetWindowLongPtrW(reinterpret_cast<HWND>(lParam), GWLP_USERDATA);
            hsAssert(data, "nil CtrlProc");
            if (data)
            {
                plStaticCtrlProc* proc = reinterpret_cast<plStaticCtrlProc*>(data);
                return proc->Color(reinterpret_cast<HDC>(wParam));
            }
            return 0;
        }
    case WM_DESTROY:
        fOwner->onClose();
        return 0;
    }
    return plWndProc::WndProc(hwnd, msg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////

plWindow::plWindow() { }

plWindow::~plWindow()
{
    for (auto it = fChildren.begin(); it != fChildren.end(); ++it)
        delete (*it);
    fChildren.clear();
    // the window is killed off by WndImp
}

void plWindow::Initialize(plWindow* parent, const plString& title, int32_t width, int32_t height)
{
    uint32_t w = (width <= 0) ? CW_USEDEFAULT : width;
    uint32_t h = (height <= 0) ? CW_USEDEFAULT : height;
    HWND p = (parent == nullptr) ? nil : parent->fImp->fHWND;
    fImp.reset(new plWndImp(this));
    fImp->Create(title.ToWchar(), w, h, p);
}

bool plWindow::Show(uint32_t x, uint32_t y)
{
    uint32_t theX = (x <= 0) ? CW_USEDEFAULT : x;
    uint32_t theY = (y <= 0) ? CW_USEDEFAULT : y;
    if (ShowWindow(fImp->fHWND, SW_SHOW) != FALSE)
    {
        SetWindowPos(fImp->fHWND, 0, theX, theY, 0, 0, SWP_NOSIZE);
        return true;
    }
    return false;
}
