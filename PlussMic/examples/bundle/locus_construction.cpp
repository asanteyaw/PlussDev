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

  // A default Locus is undefined -- the same handle-semantics as DataFrame/Catalog.
  Locus empty_locus;
  std::cout << "default defined: " << empty_locus.defined() << '\n';
  std::cout << "default empty: " << empty_locus.empty() << '\n';

  // Construct from name/Catalog pairs, as a vector or a brace list.
  Catalog customers(Streams{
      {"id", nums({1.0, 2.0, 3.0})},
      {"name", words({"Ada", "Ben", "Cy"})},
  });
  Catalog orders(Streams{
      {"order_id", nums({10.0, 11.0, 12.0})},
      {"cust_id", nums({1.0, 1.0, 2.0})},
  });
  Locus shop({{"customers", customers}, {"orders", orders}});
  std::cout << "shop defined: " << shop.defined() << '\n';
  std::cout << "shop ntables: " << shop.ntables() << '\n';

  // from_archive: one catalog per depth slice.
  auto archive = pluss::kindle::Archive({customers, orders});
  auto from_arch = Locus::from_archive(archive);
  std::cout << "from_archive ntables: " << from_arch.ntables() << '\n';
  std::cout << "from_archive names: ";
  for (const auto& n : from_arch.keys()) std::cout << n << ' ';
  std::cout << '\n';

  // from_nexus: bridges an existing DataFrame-based Nexus into a Locus.
  pluss::bundle::Nexus nexus_ws({
      {"customers", pluss::table::DataFrame({{"id", pluss::table::Slate(pluss::make_series({1.0, 2.0}))}})},
  });
  auto from_nx = Locus::from_nexus(nexus_ws);
  std::cout << "from_nexus ntables: " << from_nx.ntables() << '\n';

  return 0;
}
