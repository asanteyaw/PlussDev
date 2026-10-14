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
  Catalog items(Streams{{"sku", words({"a", "b"})}});
  Locus shop({{"customers", customers}, {"orders", orders}, {"items", items}});

  std::cout << "keys: ";
  for (const auto& n : shop.keys()) std::cout << n << ' ';
  std::cout << '\n';

  std::cout << "has(orders): " << shop.has("orders") << '\n';
  std::cout << "has(nope): " << shop.has("nope") << '\n';

  std::cout << "catalog(\"orders\").nkeys(): " << shop.catalog("orders").nkeys() << '\n';
  std::cout << "catalog(0).has(id): " << shop.catalog(0).has("id") << '\n';
  std::cout << "catalog(-1).has(sku): " << shop.catalog(-1).has("sku") << '\n';

  std::cout << "get(\"items\") has value: " << shop.get("items").has_value() << '\n';
  std::cout << "get(\"nope\") has value: " << shop.get("nope").has_value() << '\n';

  try {
    (void)shop.catalog("nope");
    std::cout << "catalog(\"nope\"): did not throw\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << "catalog(\"nope\") threw: " << msg.substr(0, msg.find('\n')) << '\n';
  }
  try {
    (void)shop.catalog(99);
    std::cout << "catalog(99): did not throw\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << "catalog(99) threw: " << msg.substr(0, msg.find('\n')) << '\n';
  }

  // catalogs() returns every member, in construction/names order.
  std::cout << "catalogs().size(): " << shop.catalogs().size() << '\n';

  return 0;
}
