// Persist a workspace and bring it back: tables, key declarations and relationships travel together, so the next session starts with the same
// workspace and passes the same checks.
#include <pluss/pluss.h>

#include <filesystem>
#include <iostream>

using namespace pluss::table;
using namespace pluss::bundle;
using pluss::relational::RelationshipCardinality;

int main() {
  auto customers = DataFrame({{"customer_id", pluss::make_series({1.0, 2.0, 3.0})}, {"name", pluss::make_note({"Ada", "Bola", "Chen"})}});
  auto orders = DataFrame({{"order_id", pluss::make_series({100.0, 101.0, 102.0})}, {"customer_id", pluss::make_series({1.0, 2.0, 2.0})}, {"amount", pluss::make_series({50.0, 75.0, 20.0})}});
  pluss::relational::TableMeta cm, om;
  cm.primary_key = {"customer_id"};
  om.primary_key = {"order_id"};
  cm.source = "crm export 2026-11-13";
  auto workspace = Nexus({{"customers", customers}, {"orders", orders}}).meta("customers", cm).meta("orders", om)
                       .add_relationship({.name = "customer_orders", .left_table = "customers", .right_table = "orders", .keys = {{"customer_id", "customer_id"}}, .cardinality = RelationshipCardinality::OneToMany});
  std::cout << "before: " << workspace.ntables() << " tables, " << workspace.nrelationships() << " relationship(s)\n";

  // 1. Save the whole workspace into a directory.
  const auto directory = std::filesystem::temp_directory_path() / "pluss_guide_workspace";
  std::filesystem::remove_all(directory);
  workspace.save(directory.string());

  // 2. Load it back: a new object, with everything declared.
  auto restored = Nexus::read(directory.string());
  std::cout << "after:  " << restored.ntables() << " tables, " << restored.nrelationships() << " relationship(s)\n";

  // 3. It behaves identically: the same pull gives the same table, and the declared source comes back with the metadata.
  auto a = workspace.pull("customers").take({"name"}).agg("orders").sum("amount").end().collect();
  auto b = restored.pull("customers").take({"name"}).agg("orders").sum("amount").end().collect();
  std::cout << "pull identical after the round trip: " << (a.equals(b) ? "yes" : "no") << "\n";
  std::cout << restored.pull("customers").take({"name"}).agg("orders").sum("amount").end().collect().to_string() << "\n";
  std::cout << "source of the customers table: " << restored.table_meta("customers")->source.value_or("(lost)") << "\n";
  std::filesystem::remove_all(directory);
}
