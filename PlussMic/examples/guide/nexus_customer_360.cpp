// A customer feature table from several related tables: declare how customers, orders and payments connect once, then assemble one row
// per customer, with counts and totals pulled from the child tables.
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::table;
using namespace pluss::bundle;

int main() {
  auto customers = DataFrame({{"customer_id", pluss::make_series({1.0, 2.0, 3.0, 4.0})}, {"name", pluss::make_note({"Ada", "Bola", "Chen", "Dara"})}, {"country", pluss::make_note({"UK", "NG", "CN", "NG"})}});
  auto orders = DataFrame({{"order_id", pluss::make_series({100.0, 101.0, 102.0, 103.0, 104.0})},
                           {"customer_id", pluss::make_series({1.0, 2.0, 2.0, 1.0, 3.0})},
                           {"amount", pluss::make_series({50.0, 75.0, 20.0, 120.0, 33.0})}});
  // Declare what one row of each table is (its primary key) and how the tables connect, once. One customer has many orders: the
  // default cardinality is many-to-one (left rows share a right row), so a one-to-many link must say so.
  pluss::relational::TableMeta customer_meta, order_meta;
  customer_meta.primary_key = {"customer_id"};
  order_meta.primary_key = {"order_id"};
  auto workspace = Nexus({{"customers", customers}, {"orders", orders}})
                       .meta("customers", customer_meta)
                       .meta("orders", order_meta)
                       .add_relationship({.name = "customer_orders", .left_table = "customers", .right_table = "orders", .keys = {{"customer_id", "customer_id"}}, .cardinality = pluss::relational::RelationshipCardinality::OneToMany});
  std::cout << workspace.summary().to_string() << "\n";

  // 1. One row per customer, with the name copied from the root and two reducers over the orders table.
  auto features = workspace.pull("customers").take({"name", "country"}).agg("orders").count("order_id").sum("amount").mean("amount").end().collect();
  std::cout << features.to_string() << "\n";

  // 2. The grain is preserved: four customers in, four rows out, including Dara who has no orders at all.
  std::cout << "customers " << customers.ncases() << ", rows pulled " << features.ncases() << "\n";

  // 3. Contrast with a join, which expands to one row per order and drops customers without orders.
  auto expanded = workspace.from("customers").join("orders").pick({"name", "amount"}).collect();
  std::cout << "join rows: " << expanded.ncases() << "\n";
}
