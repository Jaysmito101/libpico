set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL " " FORCE)

add_subdirectory(vendor/glfw)

set(FREEGLUT_BUILD_DEMOS OFF CACHE BOOL " " FORCE)
set(FREEGLUT_BUILD_SHARED_LIBS OFF CACHE BOOL " " FORCE)
set(FREEGLUT_BUILD_STATIC_LIBS ON CACHE BOOL " " FORCE)
add_subdirectory(vendor/glut)


get_target_property(GLUT_SRCS freeglut_static SOURCES)
if(GLUT_SRCS)
    list(FILTER GLUT_SRCS EXCLUDE REGEX "freeglut\\.rc")
    set_property(TARGET freeglut_static PROPERTY SOURCES "${GLUT_SRCS}")
endif()

add_library(common STATIC
    src/glapp.c
)

target_include_directories(common PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_definitions(common PUBLIC
    FREEGLUT_STATIC
    FREEGLUT_LIB_PRAGMAS=0
)

target_link_libraries(common PUBLIC glfw freeglut_static)

set(PICO_COMMON_INCLUDES 
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glut/include
    CACHE INTERNAL "Pico Common Includes"
)
set(PICO_COMMON_LIBS common CACHE INTERNAL "Pico Common Libs")

