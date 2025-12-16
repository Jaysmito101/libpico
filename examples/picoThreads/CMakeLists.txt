add_executable(picoThreadsExample main.c)

target_include_directories(picoThreadsExample PRIVATE ../../include)

if (WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS) 
endif()

if(MSVC)
    target_compile_options(picoThreadsExample PRIVATE /W4 /WX)
elseif(UNIX AND NOT APPLE)
    target_compile_options(picoThreadsExample PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
else()
    target_compile_options(picoThreadsExample PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
endif()

if (UNIX OR APPLE)
    target_link_libraries(picoThreadsExample PRIVATE pthread)
endif()