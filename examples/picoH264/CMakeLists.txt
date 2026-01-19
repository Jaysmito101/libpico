add_executable(picoH264Example main.c)

target_include_directories(picoH264Example PRIVATE ../../include)

if (WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS) 
endif()

if(MSVC)
    target_compile_options(picoH264Example PRIVATE /W4 /WX)
elseif(UNIX AND NOT APPLE)
    target_compile_options(picoH264Example PRIVATE -Wall -Wextra -Wpedantic -Werror)
else()
    target_compile_options(picoH264Example PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
