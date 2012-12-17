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

#ifdef HS_BUILD_FOR_WIN32
#   include <shellapi.h>
#endif
#pragma hdrstop

#include "plApp.h"

plArgParser::plArgParser(const char** argv, int argc)
{
    std::pair<plString, plString> assemble;
    for (int i = 0; i < argc; ++i)
    {
        plString arg(argv[i]);
        ProcessArgument(arg, assemble);
    }
}

#ifdef HS_BUILD_FOR_WIN32
plArgParser::plArgParser(const wchar_t* cmdLine)
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(cmdLine, &argc);
    hsAssert(argv, "failed to convert pCmdLine to argv");

    std::pair<plString, plString> assemble;
    for (int i = 0; i < argc; ++i)
    {
        plString arg = plString::FromWchar(argv[i]);
        ProcessArgument(arg, assemble);
    }
}
#endif

void plArgParser::ProcessArgument(const plString& arg, std::pair<plString, plString>& curr)
{
    char first = arg.CharAt(0);
    bool isArgName = (first == '/' || first == '-');
    if (curr.first.IsEmpty())
    {
        curr.first = arg.Substr(1);
    }
    else if (isArgName)
    {
        // boolean argument
        fThings[arg.Substr(1)] = "1";
    }
    else
    {
        // standard argument
        curr.second = arg;
        fThings.insert(curr);
        curr = std::make_pair("", ""); // clear out accumulator
    }
}
