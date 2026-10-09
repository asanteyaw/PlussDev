#include <pluss/bi/session.h>
#include <pluss/bi/specs.h>

#include <iostream>
#include <string>
#include <variant>

namespace bi = pluss::bi;

int main() {
  std::cout << std::boolalpha;
  // A cleared key reads as absent. It is not the same as a key that holds an empty text value.
  bi::Session session;
  session.set_param("note", std::string(""));
  std::cout << "empty text is present: " << session.has_param("note") << '\n';
  std::cout << "empty text reads as a value: " << session.param("note").has_value() << '\n';
  session.clear_param("note");
  std::cout << "cleared key is present: " << session.has_param("note") << '\n';
  std::cout << "cleared key reads as a value: " << session.param("note").has_value() << '\n';
  return 0;
}
