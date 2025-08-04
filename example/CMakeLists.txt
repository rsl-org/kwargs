project(rsl)

function(DEFINE_EXAMPLE TARGET)
  add_executable(${TARGET} "${TARGET}.cpp")
  target_link_libraries(${TARGET} PRIVATE kwargs)
endfunction()

DEFINE_EXAMPLE(simple)
DEFINE_EXAMPLE(arguments)
DEFINE_EXAMPLE(printing)