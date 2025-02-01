target_sources(kwargs_tests PRIVATE main.cpp)

add_subdirectory(parsers)
add_subdirectory(kwargs)

target_sources(kwargs_tests PRIVATE format.cpp)