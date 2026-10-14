#include "pluss/pluss.h"
#include "pluss/tils.h"
#include <iostream>

using pluss::bundle::Locus;
using pluss::bundle::locus::RelationshipCardinality;
using pluss::bundle::locus::RelationshipValidationStatus;
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
// customers 1..3; orders 10..13 (order 13 points at a customer that does not exist).
Locus make_shop() {
  Catalog customers(Streams{{"id", nums({1.0, 2.0, 3.0})}, {"name", words({"Ada", "Ben", "Cy"})}});
  Catalog orders(Streams{{"order_id", nums({10.0, 11.0, 12.0, 13.0})},
                         {"cust_id", nums({1.0, 1.0, 2.0, 9.0})},
                         {"amount", nums({5.0, 7.0, 11.0, 13.0})}});
  return Locus({{"customers", customers}, {"orders", orders}});
}
} // namespace

int main() {
  std::cout << std::boolalpha;
  auto shop = make_shop().link("orders", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne);

  // audit(): one LinkAudit per declared relationship.
  const auto audits = shop.audit();
  std::cout << "audits.size(): " << audits.size() << '\n';
  const auto& a = audits[0];
  std::cout << "left_unique (orders.cust_id): " << a.left_unique << '\n';
  std::cout << "right_unique (customers.id): " << a.right_unique << '\n';
  std::cout << "left_rows: " << a.left_rows << " right_rows: " << a.right_rows << '\n';
  std::cout << "left_orphans (orders with no customer): " << a.left_orphans << '\n';
  std::cout << "right_orphans (customers with no orders): " << a.right_orphans << '\n';
  std::cout << "status == Warning: " << (a.status == RelationshipValidationStatus::Warning) << '\n';

  // audit(name): the same check for one named relationship.
  const auto one = shop.audit("orders->customers");
  std::cout << "audit(name).left_orphans: " << one.left_orphans << '\n';

  // validate(): throws only on Invalid, not on Warning -- orphans on an
  // optional side do not fail a build.
  shop.validate();
  std::cout << "validate() on warning-only orphans: did not throw\n";

  // validate_keys: are a table's key columns present and unique?
  std::cout << "validate_keys(customers, {id}): " << shop.validate_keys("customers", {"id"}) << '\n';
  std::cout << "validate_keys(orders, {cust_id}): " << shop.validate_keys("orders", {"cust_id"}) << '\n';

  // duplicates / detect_duplicates: records whose key repeats.
  const auto dup_rows = shop.duplicates("orders", {"cust_id"});
  std::cout << "duplicates(orders, cust_id) rows: " << dup_rows.get("order_id")->size() << '\n';

  // detect_orphans: records on one side of a link with no partner on the other.
  const auto orphan_orders = shop.detect_orphans("orders->customers", pluss::bundle::locus::RelationshipSide::Left);
  std::cout << "detect_orphans(left) rows: " << orphan_orders.get("order_id")->size() << '\n';

  // check / validate_relationship: a single relationship's pass/fail.
  std::cout << "validate_relationship(orders->customers): " << shop.validate_relationship("orders->customers") << '\n';
  std::cout << "check(orders->customers) (alias): " << shop.check("orders->customers") << '\n';

  return 0;
}
