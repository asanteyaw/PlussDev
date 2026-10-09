#include <pluss/bi/session.h>
#include <pluss/bi/specs.h>

#include <iostream>
#include <string>
#include <variant>

namespace bi = pluss::bi;

template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

int main() {
  std::cout << std::boolalpha;
  // A session is a value. A copy keeps its own state, so changing one does not change the other.
  bi::Session original;
  original.set_param("region", std::string("north"));
  bi::Session copy = original;
  copy.set_param("region", std::string("south"));
  std::cout << "original region: " << std::get<std::string>(*original.param("region")) << '\n';
  std::cout << "copy region: " << std::get<std::string>(*copy.param("region")) << '\n';
  std::cout << "copy version after its own change: " << copy.version() << '\n';
  std::cout << "original version unchanged: " << (original.version() == 1) << '\n';
  return 0;
}
