add_executable(picoCliExample main.c)

target_include_directories(picoCliExample PRIVATE ../../include)

if(MSVC)
    target_compile_options(picoCliExample PRIVATE /W4 /WX)
else()
    target_compile_options(picoCliExample PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
