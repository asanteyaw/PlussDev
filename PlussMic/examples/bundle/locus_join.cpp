#include "pluss/pluss.h"
#include "pluss/tils.h"
#include <iostream>

using pluss::bundle::Locus;
using pluss::bundle::locus::JoinHow;
using pluss::bundle::locus::JoinSpec;
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
  return Locus({{"customers", customers}, {"orders", orders}});
}
} // namespace

int main() {
  std::cout << std::boolalpha;
  const auto shop = make_shop().link("orders", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne);

  // join(left, right): the declared link supplies the keys, default Inner --
  // the orphan order (13, customer 9 does not exist) is dropped.
  const auto joined = shop.join("orders", "customers");
  std::cout << "inner join rows: " << joined.get("amount")->size() << '\n';

  // A left join keeps the orphan, with a null on the customer side.
  JoinSpec left_spec;
  left_spec.how = JoinHow::Left;
  const auto kept = shop.join("orders", "customers", left_spec);
  std::cout << "left join rows: " << kept.get("amount")->size() << '\n';

  // Explicit keys join without any declared link.
  JoinSpec explicit_keys;
  explicit_keys.keys = {{"cust_id", "id"}};
  const auto manual = make_shop().join("orders", "customers", explicit_keys);
  std::cout << "explicit-key join rows: " << manual.get("amount")->size() << '\n';

  // No keys and no declared link: refused.
  try {
    (void)make_shop().join("orders", "customers");
    std::cout << "unlinked join: did not throw\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << "unlinked join threw: " << msg.substr(0, msg.find('\n')) << '\n';
  }

  // join_detailed: the same join, plus a report of what was matched, left
  // without a partner, or omitted.
  const auto detailed = shop.join_detailed("orders", "customers");
  std::cout << "join_detailed matched: " << detailed.catalog.get("amount")->size() << '\n';
  std::cout << "join_detailed report: " << detailed.report.summary() << '\n';

  return 0;
}
