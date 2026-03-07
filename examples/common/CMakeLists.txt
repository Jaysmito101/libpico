set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL " " FORCE)

add_subdirectory(vendor/glfw)
add_subdirectory(vendor/glad/cmake)

glad_add_library(glad STATIC API gl:core=4.3)

add_library(common STATIC
    src/glapp.c
)


target_include_directories(common PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw/include>
)

target_link_libraries(common PUBLIC glfw glad)

set(PICO_COMMON_INCLUDES 
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw/include
    CACHE INTERNAL "Pico Common Includes"
)
set(PICO_COMMON_LIBS common CACHE INTERNAL "Pico Common Libs")

