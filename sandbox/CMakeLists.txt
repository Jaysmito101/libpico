add_executable(sandbox main.c)

target_include_directories(sandbox PRIVATE ../include)


if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS) 
    target_compile_options(sandbox PRIVATE /W4 /WX)
else()
    target_compile_options(sandbox PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
endif()