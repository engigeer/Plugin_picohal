add_library(picohal INTERFACE)

target_sources(picohal INTERFACE
 ${CMAKE_CURRENT_LIST_DIR}/picohal.c
)

target_include_directories(picohal INTERFACE ${CMAKE_CURRENT_LIST_DIR})
