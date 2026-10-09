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
  // Setting an existing key with the other type replaces the stored value and its type.
  bi::Session session;
  session.set_param("mode", 1.0);
  session.set_param("mode", std::string("fast"));
  std::cout << "mode is now text: " << std::holds_alternative<std::string>(*session.param("mode")) << '\n';
  std::cout << "still one parameter: " << (session.params().size() == 1) << '\n';
  return 0;
}
