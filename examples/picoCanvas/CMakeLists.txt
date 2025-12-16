add_executable(picoCanvasExample main.c)

target_include_directories(picoCanvasExample PRIVATE ../../include)
set(PKG_CONFIG_EXECUTABLE "/usr/bin/pkg-config")

if (WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS) 
endif()

if(MSVC)
    target_compile_options(picoCanvasExample PRIVATE /W4 /WX)
elseif(UNIX AND NOT APPLE)
    option(USE_WAYLAND "Use Wayland instead of X11" OFF)
    
    find_package(PkgConfig REQUIRED)
    
    target_link_libraries(picoCanvasExample PRIVATE m)

    if(USE_WAYLAND)
        message(STATUS "Building with Wayland support")
        target_compile_definitions(picoCanvasExample PRIVATE PICO_CANVAS_PREFER_WAYLAND)
        
        pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
        pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
        
        target_include_directories(picoCanvasExample PRIVATE ${WAYLAND_CLIENT_INCLUDE_DIRS})
        target_link_libraries(picoCanvasExample PRIVATE ${WAYLAND_CLIENT_LIBRARIES})
    else()
        message(STATUS "Building with X11 support")
        target_compile_definitions(picoCanvasExample PRIVATE PICO_CANVAS_PREFER_X11)
        
        find_package(X11 REQUIRED)
        
        target_include_directories(picoCanvasExample PRIVATE ${X11_INCLUDE_DIR})
        target_link_libraries(picoCanvasExample PRIVATE ${X11_LIBRARIES})
    endif()
    
    target_compile_options(picoCanvasExample PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
else()
    target_compile_options(picoCanvasExample PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
endif()