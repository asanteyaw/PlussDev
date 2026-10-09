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

bi::Filter region_filter(const std::string& key, const std::string& value) {
  return bi::Filter{.key = key, .feature = "region", .op = bi::FilterOp::Eq,
                    .payload = bi::FilterScalar{value}};
}

int main() {
  std::cout << std::boolalpha;
  // An empty text value is a value. It is not the same as no value.
  bi::Session session;
  session.set_param("note", std::string(""));
  const auto note = session.param("note");
  std::cout << "empty text is a value: " << note.has_value() << '\n';
  std::cout << "empty text is text: " << std::holds_alternative<std::string>(*note) << '\n';
  std::cout << "length: " << std::get<std::string>(*note).size() << '\n';
  return 0;
}
