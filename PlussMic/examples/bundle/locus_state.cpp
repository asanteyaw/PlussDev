#include "pluss/pluss.h"
#include "pluss/tils.h"
#include <iostream>

using pluss::bundle::Locus;
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
  Locus shop({{"customers", customers}, {"orders", orders}});

  std::cout << "defined: " << shop.defined() << '\n';
  std::cout << "empty: " << shop.empty() << '\n';
  std::cout << "ntables: " << shop.ntables() << '\n';
  std::cout << "nrelationships (none declared yet): " << shop.nrelationships() << '\n';
  const auto shape = shop.shape();
  std::cout << "shape: {" << shape.first << ", " << shape.second << "}\n";

  // unique: are all table names distinct? Construction itself already refuses
  // a duplicate name, so a successfully built Locus is always unique.
  std::cout << "unique: " << shop.unique() << '\n';

  const auto linked = shop.link("orders", "cust_id", "customers", "id");
  std::cout << "linked nrelationships: " << linked.nrelationships() << '\n';
  std::cout << "linked shape: {" << linked.shape().first << ", " << linked.shape().second << "}\n";

  return 0;
}
