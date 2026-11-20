// The customer feature table again, in the Locus model: catalogs of streams rather than DataFrames, so the child streams keep their own
// lengths and are never forced into a rectangle. One row per customer comes out.
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::kindle::literals;
using pluss::bundle::Locus;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;
using pluss::relational::RelationshipCardinality;

int main() {
  Catalog customers({"customer_id"_k = Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0})), "name"_k = Stream(pluss::make_note({"Ada", "Bola", "Chen", "Dara"}))});
  Catalog orders({"order_id"_k = Stream(pluss::make_series({100.0, 101.0, 102.0, 103.0, 104.0})), "customer_id"_k = Stream(pluss::make_series({1.0, 2.0, 2.0, 1.0, 3.0})),
                  "amount"_k = Stream(pluss::make_series({50.0, 75.0, 20.0, 120.0, 33.0}))});
  pluss::relational::TableMeta cm, om;
  cm.primary_key = {"customer_id"};
  om.primary_key = {"order_id"};
  auto workspace = Locus({{"customers", customers}, {"orders", orders}})
                       .set_table_meta("customers", cm)
                       .set_table_meta("orders", om)
                       .link({.name = "customer_orders", .left_table = "customers", .right_table = "orders", .keys = {{"customer_id", "customer_id"}}, .cardinality = RelationshipCardinality::OneToMany, .right_optional = true});

  // 1. One record per customer, with a count and totals reduced from the orders catalog.
  auto features = workspace.pull("customers").take({"name"}).agg("orders").count("order_id").sum("amount").end().collect();
  std::cout << features << "\n";
  std::cout << "customers " << customers.nrows() << ", records pulled " << features.nrows() << "\n";

  // 2. The same audit as a Nexus. Dara has no orders; because the link declares the orders side optional (right_optional), that is not a violation.
  std::cout << workspace.audit_report().features({"name", "valid"}).to_string() << "\n";
}
