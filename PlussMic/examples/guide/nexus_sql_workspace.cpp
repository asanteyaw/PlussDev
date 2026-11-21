// Ask SQL questions across a workspace: revenue by country, the best customers, and a parameterised question, all through one query accessor.
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::table;
using namespace pluss::bundle;
using pluss::relational::RelationshipCardinality;

int main() {
  auto customers = DataFrame({{"customer_id", pluss::make_series({1.0, 2.0, 3.0, 4.0})}, {"name", pluss::make_note({"Ada", "Bola", "Chen", "Dara"})}, {"country", pluss::make_note({"UK", "NG", "CN", "NG"})}});
  auto orders = DataFrame({{"order_id", pluss::make_series({100.0, 101.0, 102.0, 103.0, 104.0})}, {"customer_id", pluss::make_series({1.0, 2.0, 2.0, 1.0, 3.0})}, {"amount", pluss::make_series({50.0, 75.0, 20.0, 120.0, 33.0})}});
  pluss::relational::TableMeta cm, om;
  cm.primary_key = {"customer_id"};
  om.primary_key = {"order_id"};
  auto workspace = Nexus({{"customers", customers}, {"orders", orders}}).meta("customers", cm).meta("orders", om)
                       .add_relationship({.name = "customer_orders", .left_table = "customers", .right_table = "orders", .keys = {{"customer_id", "customer_id"}}, .cardinality = RelationshipCardinality::OneToMany});

  // 1. Revenue per country: tables are addressed by name and joined in the statement.
  auto by_country = workspace.query().execute(
      "SELECT customers.country AS country, COUNT(*) AS orders, SUM(orders.amount) AS revenue "
      "FROM customers JOIN orders ON customers.customer_id = orders.customer_id GROUP BY customers.country ORDER BY revenue DESC");
  std::cout << by_country.to_string() << "\n";

  // 2. Customers by total spend, including those who never ordered (left join).
  auto spend = workspace.query().execute(
      "SELECT customers.name AS name, COALESCE(SUM(orders.amount), 0) AS spent FROM customers LEFT JOIN orders ON customers.customer_id = orders.customer_id "
      "GROUP BY customers.name ORDER BY spent DESC");
  std::cout << spend.to_string() << "\n";

  // 3. A parameterised question: orders above a threshold, with the threshold passed as a value.
  DataFrame::SqlOptions options;
  options.params["minimum"] = 60.0;
  auto large = workspace.query(options).execute("SELECT orders.order_id, customers.name, orders.amount FROM orders JOIN customers ON customers.customer_id = orders.customer_id WHERE orders.amount >= {minimum} ORDER BY orders.amount DESC");
  std::cout << large.to_string() << "\n";
}
