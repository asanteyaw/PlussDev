add_executable(eng_robot_test robot_test.cpp)
target_link_libraries(eng_robot_test PRIVATE pluss plussqit_codem)
add_test(NAME eng_robot_test COMMAND eng_robot_test)
