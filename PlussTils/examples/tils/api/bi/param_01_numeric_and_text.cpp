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
  // param returns the stored value as a variant. Check its alternative to know the type.
  bi::Session session;
  session.set_param("threshold", 2.5);
  session.set_param("region", std::string("north"));
  const auto threshold = session.param("threshold");
  const auto region = session.param("region");
  std::cout << "threshold holds a number: " << std::holds_alternative<double>(*threshold) << '\n';
  std::cout << "region holds text: " << std::holds_alternative<std::string>(*region) << '\n';
  std::cout << "threshold value: " << std::get<double>(*threshold) << '\n';
  return 0;
}
