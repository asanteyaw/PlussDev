#include <pluss/tils/kit/output.h>

#include <iostream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  // Without strict checking, an extra argument is not written, and the line is otherwise complete.
  out.writelnf("order {} shipped to {}", 1001, "Lagos", "express");
  return 0;
}
