add_executable(eng_power_test power_test.cpp)
target_link_libraries(eng_power_test PRIVATE pluss plussqit_codem)
add_test(NAME eng_power_test COMMAND eng_power_test)
