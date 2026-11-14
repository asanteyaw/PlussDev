add_executable(eng_comms_test comms_test.cpp)
target_link_libraries(eng_comms_test PRIVATE pluss plussqit_codem)
add_test(NAME eng_comms_test COMMAND eng_comms_test)
