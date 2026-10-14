#include "pluss/pluss.h"
#include "pluss/tils.h"
#include <iostream>

using pluss::bundle::Locus;
using pluss::bundle::locus::Relationship;
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
  Catalog orders(Streams{{"order_id", nums({10.0, 11.0, 12.0, 13.0})},
                         {"cust_id", nums({1.0, 1.0, 2.0, 9.0})},
                         {"amount", nums({5.0, 7.0, 11.0, 13.0})}});
  Catalog items(Streams{{"item_order", nums({10.0, 10.0, 11.0, 12.0, 12.0})}, {"sku", words({"a", "b", "c", "d", "e"})}});
  return Locus({{"customers", customers}, {"orders", orders}, {"items", items}});
}
} // namespace

int main() {
  std::cout << std::boolalpha;
  auto shop = make_shop();

  // link(left, left_key, right, right_key, cardinality): the four-argument key form.
  shop = shop.link("orders", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne);
  // link(left, right, key, cardinality): the three-argument form, when the key name is shared.
  shop = shop.link("items", "item_order", "orders", "order_id", RelationshipCardinality::ManyToOne);
  std::cout << "nrelationships: " << shop.nrelationships() << '\n';

  std::cout << "has_relationship(orders->customers): " << shop.has_relationship("orders->customers") << '\n';
  std::cout << "relationship(items->orders).cardinality == ManyToOne: "
            << (shop.relationship("items->orders")->cardinality == RelationshipCardinality::ManyToOne) << '\n';

  std::cout << "neighbors(orders): ";
  for (const auto& n : shop.neighbors("orders")) std::cout << n << ' ';
  std::cout << '\n';

  // The same pair linked again gets a distinct auto-generated name.
  const auto twice = shop.link("orders", "cust_id", "customers", "id");
  std::cout << "twice has_relationship(orders->customers#2): " << twice.has_relationship("orders->customers#2") << '\n';

  // An explicit duplicate relationship name is refused.
  Relationship dup;
  dup.name = "orders->customers";
  dup.left_table = "orders";
  dup.right_table = "customers";
  dup.keys = {{"cust_id", "id"}};
  try {
    (void)shop.link(dup);
    std::cout << "duplicate link: did not throw\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << "duplicate link threw: " << msg.substr(0, msg.find('\n')) << '\n';
  }

  // unlink removes a link without touching the catalogs.
  const auto fewer = shop.unlink("items->orders");
  std::cout << "after unlink, has_relationship(items->orders): " << fewer.has_relationship("items->orders") << '\n';
  std::cout << "shop still has_relationship(items->orders): " << shop.has_relationship("items->orders") << '\n';

  // path: the shortest chain of links between two tables, as a sequence of hops.
  const auto to_customers = shop.path("items", "customers");
  std::cout << "path(items, customers) hops: " << to_customers->steps.size() << '\n';
  std::cout << "hop 0: " << to_customers->steps[0].from_table << " -> " << to_customers->steps[0].to_table << '\n';
  const auto self = shop.path("orders", "orders");
  std::cout << "path(orders, orders) hops: " << self->steps.size() << '\n';
  const auto isolated = shop.with("island", shop.catalog("customers"));
  std::cout << "path(island, orders) has value: " << isolated.path("island", "orders").has_value() << '\n';

  return 0;
}
