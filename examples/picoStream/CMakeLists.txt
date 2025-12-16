add_executable(picoStreamExample main.c)

target_include_directories(picoStreamExample PRIVATE ../../include)
set(PKG_CONFIG_EXECUTABLE "/usr/bin/pkg-config")

if (WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS) 
endif()

if(MSVC)
    target_compile_options(picoStreamExample PRIVATE /W4 /WX)
elseif(UNIX AND NOT APPLE)
    target_compile_options(picoStreamExample PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
else()
    target_compile_options(picoStreamExample PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
endif()