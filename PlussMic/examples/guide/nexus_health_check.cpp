// Check a multi-table workspace before trusting any answer from it: key integrity, duplicate keys, orphaned child rows and the state of every
// declared relationship, in one pass.
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::table;
using namespace pluss::bundle;

int main() {
  // Customers with a duplicated id (2) and orders that include one for a customer who does not exist (9).
  auto customers = DataFrame({{"customer_id", pluss::make_series({1.0, 2.0, 2.0, 3.0})}, {"name", pluss::make_note({"Ada", "Bola", "Bola (dup)", "Chen"})}});
  auto orders = DataFrame({{"order_id", pluss::make_series({100.0, 101.0, 102.0, 103.0})}, {"customer_id", pluss::make_series({1.0, 2.0, 9.0, 3.0})}, {"amount", pluss::make_series({50.0, 75.0, 20.0, 33.0})}});
  pluss::relational::TableMeta customer_meta, order_meta;
  customer_meta.primary_key = {"customer_id"};
  order_meta.primary_key = {"order_id"};
  auto workspace = Nexus({{"customers", customers}, {"orders", orders}})
                       .set_table_meta("customers", customer_meta)
                       .set_table_meta("orders", order_meta)
                       .add_relationship({.name = "customer_orders", .left_table = "orders", .right_table = "customers", .keys = {{"customer_id", "customer_id"}}});

  // 1. The command-center view: one row per table and per relationship, with what is wrong.
  std::cout << workspace.audit().features({"name", "right_duplicate_rows", "left_orphans", "valid"}).to_string() << "\n";

  // 2. Table level: is the declared key complete and unique?
  std::cout << "customers key valid: " << (workspace.validate_keys("customers") ? "yes" : "no") << "\n";
  std::cout << "orders key valid:    " << (workspace.validate_keys("orders") ? "yes" : "no") << "\n";

  // 3. Row level: which rows break the rules?
  std::cout << "duplicated customer rows:\n" << workspace.detect_duplicates("customers").to_string() << "\n";
  std::cout << "orders without a customer:\n" << workspace.detect_orphans("customer_orders").to_string() << "\n";
}
