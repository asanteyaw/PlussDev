// Walk several relationships to answer a business question: revenue by product category from customers, orders, order lines and products.
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::table;
using namespace pluss::bundle;
using pluss::relational::RelationshipCardinality;

int main() {
  auto orders = DataFrame({{"order_id", pluss::make_series({1.0, 2.0, 3.0})}, {"customer", pluss::make_note({"Ada", "Bola", "Ada"})}});
  auto lines = DataFrame({{"line_id", pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})}, {"order_id", pluss::make_series({1.0, 1.0, 2.0, 3.0, 3.0})},
                          {"sku", pluss::make_note({"A1", "B2", "A1", "C3", "B2"})}, {"qty", pluss::make_series({2.0, 1.0, 5.0, 1.0, 3.0})}});
  auto products = DataFrame({{"sku", pluss::make_note({"A1", "B2", "C3"})}, {"category", pluss::make_note({"tools", "toys", "tools"})}, {"price", pluss::make_series({10.0, 25.0, 40.0})}});
  pluss::relational::TableMeta o, l, p;
  o.primary_key = {"order_id"};
  l.primary_key = {"line_id"};
  p.primary_key = {"sku"};
  auto workspace = Nexus({{"orders", orders}, {"lines", lines}, {"products", products}})
                       .set_table_meta("orders", o).set_table_meta("lines", l).set_table_meta("products", p)
                       .add_relationship({.name = "order_lines", .left_table = "orders", .right_table = "lines", .keys = {{"order_id", "order_id"}}, .cardinality = RelationshipCardinality::OneToMany})
                       .add_relationship({.name = "line_product", .left_table = "lines", .right_table = "products", .keys = {{"sku", "sku"}}, .cardinality = RelationshipCardinality::ManyToOne});

  // 1. The path from orders to products is two hops. Walk it and keep the fields that matter.
  auto walked = workspace.from("orders").join("lines").join("products").pick({"customer", "category", "qty", "price"}).collect();
  std::cout << walked.to_string() << "\n";

  // 2. Revenue is quantity times price per line; add it and total by category.
  auto revenue = walked.assign({{"revenue", [](const DataFrame &f) { return f.features("qty") * f.features("price"); }}});
  auto by_category = revenue.group({.dropnull = true, .sort = true}).by({"category"}).agg({{"lines", "qty", DataFrame::GroupOp::Count}, {"revenue", "revenue", DataFrame::GroupOp::Sum}});
  std::cout << by_category.to_string() << "\n";

  // 3. And by customer: the same walked table answers any question about these three tables.
  auto by_customer = revenue.group({.dropnull = true, .sort = true}).by({"customer"}).agg({{"revenue", "revenue", DataFrame::GroupOp::Sum}});
  std::cout << by_customer.to_string() << "\n";
}
