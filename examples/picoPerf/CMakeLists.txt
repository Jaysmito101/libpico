add_executable(picoPerfExample main.c)

target_include_directories(picoPerfExample PRIVATE ../../include)

if (WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS) 
endif()

if(MSVC)
    target_compile_options(picoPerfExample PRIVATE /W4 /WX)
elseif(UNIX AND NOT APPLE)
    target_compile_options(picoPerfExample PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
else()
    target_compile_options(picoPerfExample PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
endif()

if (UNIX OR APPLE)
    target_link_libraries(picoPerfExample PRIVATE m)
endif()