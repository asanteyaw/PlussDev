#include "pluss/pluss.h"
#include "pluss/tils.h"
#include <iostream>
#include <sstream>

using pluss::bundle::Locus;
using pluss::bundle::locus::RelationshipCardinality;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;
using Streams = std::vector<std::pair<std::string, Stream>>;

namespace {
Stream nums(const std::vector<double>& v) {
  return Stream(pluss::make_series(v, {static_cast<std::int64_t>(v.size())}));
}
Stream words(const std::vector<std::string>& v) {
  return Stream(pluss::make_note(v, {static_cast<std::int64_t>(v.size())}));
}
} // namespace

int main() {
  std::cout << std::boolalpha;

  Catalog customers(Streams{{"id", nums({1.0, 2.0, 3.0})}, {"name", words({"Ada", "Ben", "Cy"})}});
  Catalog orders(Streams{{"order_id", nums({10.0, 11.0, 12.0})}, {"cust_id", nums({1.0, 1.0, 2.0})}});
  const auto shop = Locus({{"customers", customers}, {"orders", orders}})
                         .link("orders", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne);

  // to_string() names the tables and the declared links.
  const auto text = shop.to_string();
  std::cout << "to_string mentions customers: " << (text.find("customers") != std::string::npos) << '\n';
  std::cout << "to_string mentions orders: " << (text.find("orders") != std::string::npos) << '\n';
  std::cout << "to_string mentions the link: " << (text.find("orders->customers") != std::string::npos) << '\n';

  // show(os) writes to_string() plus a trailing newline -- meant for
  // terminal-style printing, not for byte-identical reuse of the string.
  std::ostringstream shown;
  shop.show(shown);
  std::cout << "show(os) == to_string() + \"\\n\": " << (shown.str() == text + "\n") << '\n';

  // operator<< is the plain stream form, with no added newline -- it matches
  // to_string() exactly, the same way std::cout << someLocus behaves.
  std::ostringstream streamed;
  streamed << shop;
  std::cout << "operator<< matches to_string() exactly: " << (streamed.str() == text) << '\n';

  return 0;
}
