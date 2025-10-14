add_executable(picoPerfExample main.c)

target_include_directories(picoPerfExample PRIVATE ../../include)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS) 
    target_compile_options(picoPerfExample PRIVATE /W4 /WX)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(picoPerfExample PRIVATE m)
    target_compile_options(picoPerfExample PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
else()
    target_link_libraries(picoPerfExample PRIVATE m)
    target_compile_options(picoPerfExample PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
endif()