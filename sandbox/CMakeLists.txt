add_executable(sandbox main.c)

target_include_directories(sandbox PRIVATE ../include)

if (WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS) 
endif()

if(MSVC)
    target_compile_options(sandbox PRIVATE /W4 /WX)
elseif(UNIX AND NOT APPLE)
    target_compile_options(sandbox PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
else()
    target_compile_options(sandbox PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
endif()