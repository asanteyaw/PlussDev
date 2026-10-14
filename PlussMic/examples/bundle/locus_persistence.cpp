#include "pluss/pluss.h"
#include "pluss/tils.h"
#include <filesystem>
#include <iostream>

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
Locus make_shop() {
  Catalog customers(Streams{{"id", nums({1.0, 2.0, 3.0})}, {"name", words({"Ada", "Ben", "Cy"})}});
  Catalog orders(Streams{{"order_id", nums({10.0, 11.0, 12.0})}, {"cust_id", nums({1.0, 1.0, 2.0})},
                         {"amount", nums({5.0, 7.0, 11.0})}});
  return Locus({{"customers", customers}, {"orders", orders}});
}
} // namespace

int main() {
  std::cout << std::boolalpha;
  namespace fs = std::filesystem;
  const auto shop = make_shop().link("orders", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne);

  const auto dir = fs::temp_directory_path() / "locus_docs_persistence_example";
  fs::remove_all(dir);

  // save(): ragged-native -- each catalog is written stream by stream, so
  // extents, nulls, and kinds round-trip exactly. Links and table metadata
  // round-trip too.
  shop.save(dir.string());
  std::cout << "save() wrote a directory: " << fs::exists(dir) << '\n';

  // read(): the matching load for a Locus-written (or Nexus-written) store.
  const auto back = Locus::read(dir.string());
  std::cout << "read() keys match: " << (back.keys() == shop.keys()) << '\n';
  std::cout << "read() nrelationships: " << back.nrelationships() << '\n';
  std::cout << "restored join still works, rows: " << back.join("orders", "customers").get("amount")->size() << '\n';

  fs::remove_all(dir);
  return 0;
}
