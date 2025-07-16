file(GLOB_RECURSE MASKING_SOURCES CONFIGURE_DEPENDS *.c)

add_library(dom_lib_src
    ${MASKING_SOURCES}
)

target_include_directories(dom_lib_src
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)