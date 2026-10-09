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
  // A parameter holds either a number or text. The overload you call picks the type it stores.
  bi::Session session;
  session.set_param("threshold", 2.5);
  session.set_param("region", std::string_view("north"));
  std::cout << "threshold is numeric: " << std::holds_alternative<double>(*session.param("threshold")) << '\n';
  std::cout << "region is text: " << std::holds_alternative<std::string>(*session.param("region")) << '\n';
  std::cout << "threshold value: " << std::get<double>(*session.param("threshold")) << '\n';
  return 0;
}
