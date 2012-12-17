include_directories("../../CoreLib")
include_directories("../../NucleusLib/inc")
include_directories("../../NucleusLib")
include_directories("./")

set(plWndApp_SOURCES
    plArgParser.cpp
    plControlBase.cpp
)

set(plWndApp_HEADERS
    plApp.h
    plControls.h
    plWindow.h
)

# Add platform specific files here
if (WIN32)
    set(plWndApp_HEADERS
        ${plWndApp_HEADERS}
        Win32/plWndProc.h
    )

    set(plWndApp_SOURCES
        ${plWndApp_SOURCES}
        Win32/plControls.cpp
        Win32/plWindow.cpp
    )
endif(WIN32)

use_precompiled_header(Pch.h Pch.cpp plWndApp_HEADERS plWndApp_SOURCES)
add_library(plWndApp STATIC ${plWndApp_SOURCES} ${plWndApp_HEADERS})

if (WIN32)
    target_link_libraries(plWndApp comctl32.lib)
endif ()

source_group("Source Files" FILES ${plWndApp_SOURCES})
source_group("Header Files" FILES ${plWndApp_HEADERS})
