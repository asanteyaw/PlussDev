
# This CMakeLists.txt is for the external PlussQit module.
# It is intended for examples and demonstration code that use the internal PlussQit utilities.
# All core logic is in include/internals/PlussQit (see CodeM, CodeL, CodeN, etc.).

cmake_minimum_required(VERSION 3.28)
project(PlussQitExamples LANGUAGES CXX)

# Add examples and tests as subdirectories
add_subdirectory(examples)
add_subdirectory(test)