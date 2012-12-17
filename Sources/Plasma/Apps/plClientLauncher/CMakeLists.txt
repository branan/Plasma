include_directories("../../CoreLib")
include_directories("../../NucleusLib")
include_directories("../../PubUtilLib")

set(plClientLauncher_SOURCES
    main.cpp
)

add_executable(plClientLauncher WIN32 ${plClientLauncher_SOURCES})
target_link_libraries(plClientLauncher CoreLib)
target_link_libraries(plClientLauncher plWndApp)

if(USE_VLD)
    target_link_libraries(plClientLauncher ${VLD_LIBRARY})
endif()

source_group("Source Files" FILES ${plClientLauncher_SOURCES})
