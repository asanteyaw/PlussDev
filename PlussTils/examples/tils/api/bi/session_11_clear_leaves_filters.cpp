#include <pluss/bi/session.h>
#include <pluss/bi/specs.h>

#include <iostream>
#include <string>
#include <variant>

namespace bi = pluss::bi;

int main() {
  std::cout << std::boolalpha;
  // Clearing a parameter leaves filters in place, and clearing a filter leaves parameters in place.
  bi::Session session;
  session.set_param("threshold", 2.5);
  session.set_filter(bi::Filter{.key = "region", .feature = "region", .op = bi::FilterOp::Eq,
                                .payload = bi::FilterScalar{std::string("north")}});
  session.clear_param("threshold");
  std::cout << "filter survives a parameter clear: " << session.has_filter("region") << '\n';
  std::cout << "filters after clear: " << session.filters().size() << '\n';
  return 0;
}
