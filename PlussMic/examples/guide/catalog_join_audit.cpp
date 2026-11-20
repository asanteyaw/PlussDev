// Join customers to orders safely: audit the keys first (duplicates, missing keys, orphans), then join and read the report of what the join did.
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::kindle::literals;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

int main() {
  Catalog customers({"customer"_k = Stream(pluss::make_note({"ada", "grace", "alan", "edsger"})), "country"_k = Stream(pluss::make_note({"UK", "US", "UK", "NL"}))});
  Catalog orders({"order_id"_k = Stream(pluss::make_series({101, 102, 103, 104, 105})),
                  "customer"_k = Stream(pluss::make_note({"ada", "grace", "ada", "alan", "barbara"})),
                  "amount"_k = Stream(pluss::make_series({120.0, 75.5, 40.0, 310.0, 22.5}))});

  // 1. Audit before joining: is each side unique on the key, are there records without a key, and how many have no partner?
  auto audit = customers.audit_keys(orders, {"customer"}, {"customer"});
  std::cout << "customers: " << audit.left_records << " records, unique " << (audit.left_unique ? "yes" : "no") << ", orphans " << audit.left_orphans << "\n";
  std::cout << "orders:    " << audit.right_records << " records, unique " << (audit.right_unique ? "yes" : "no") << ", orphans " << audit.right_orphans << "\n";

  // 2. The join. Left keeps every customer; "validate" states the expected relationship so a surprise raises an error instead of
  //    silently multiplying rows.
  pluss::kindle::CatalogJoinSpec spec;
  spec.how = pluss::relational::JoinHow::Left;
  spec.left_keys = {"customer"};
  spec.right_keys = {"customer"};
  auto joined = customers.join(orders, spec);
  std::cout << joined << "\n";

  // 3. join_detailed returns the same result with a report: nothing is lost uncounted.
  auto detailed = customers.join_detailed(orders, spec);
  std::cout << "report: " << detailed.report.summary() << "\n";

  // 4. A wrong expectation is an error, not a quiet duplication: customers and orders are one-to-many, not one-to-one.
  spec.validate = pluss::relational::RelationshipCardinality::OneToOne;
  try {
    (void)customers.join(orders, spec);
    std::cout << "accepted as one-to-one\n";
  } catch (const pluss::Error &e) {
    std::cout << "rejected: " << e.message() << "\n";
  }
}
