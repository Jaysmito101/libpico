add_executable(picoLogExample main.c)

target_include_directories(picoLogExample PRIVATE ../../include)

if (WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS) 
endif()

if(MSVC)
    target_compile_options(picoLogExample PRIVATE /W4 /WX)
elseif(UNIX AND NOT APPLE)
    target_compile_options(picoLogExample PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
else()
    target_compile_options(picoLogExample PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
endif()