add_executable(picoAudioExample main.c)

if (WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS) 
endif()


target_include_directories(picoAudioExample PRIVATE ../../include)

if (APPLE)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    if (AUDIOTOOLBOX_FRAMEWORK AND COREFOUNDATION_FRAMEWORK)
        target_link_libraries(picoAudioExample PRIVATE ${AUDIOTOOLBOX_FRAMEWORK} ${COREFOUNDATION_FRAMEWORK})
    endif()
endif()

if(MSVC)
    target_compile_options(picoAudioExample PRIVATE /W4 /WX)
elseif(UNIX AND NOT APPLE)
    target_compile_options(picoAudioExample PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
else()
    target_compile_options(picoAudioExample PRIVATE -Wall -Wextra -Wpedantic -Werror -Woverlength-strings)
endif()

