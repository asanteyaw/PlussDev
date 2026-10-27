add_executable(eng_fluid_test fluid_test.cpp)
target_link_libraries(eng_fluid_test PRIVATE pluss plussqit_codem)
add_test(NAME eng_fluid_test COMMAND eng_fluid_test)
