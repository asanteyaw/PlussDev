// Combine two tables: customers and their orders. Join them, total the orders per customer, find customers who never ordered, and do the same in SQL.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;

int main() {
  auto customers = DataFrame({
      {"customer", pluss::make_note({"ada", "grace", "alan", "edsger"})},
      {"country", pluss::make_note({"UK", "US", "UK", "NL"})},
  });
  auto orders = DataFrame({
      {"order_id", pluss::make_series({101, 102, 103, 104, 105})},
      {"customer", pluss::make_note({"ada", "grace", "ada", "alan", "ada"})},
      {"amount", pluss::make_series({120.0, 75.5, 40.0, 310.0, 22.5})},
  });

  // 1. SQL has the vocabulary for relational questions. The calling frame is `df`; other frames are bound by name.
  DataFrame::SqlOptions options;
  options.sources.push_back({"orders", &orders});
  auto spend = customers.sql(options).execute(
      "SELECT df.customer, df.country, COUNT(orders.order_id) AS orders, COALESCE(SUM(orders.amount), 0) AS spent "
      "FROM df LEFT JOIN orders ON orders.customer = df.customer "
      "GROUP BY df.customer, df.country ORDER BY spent DESC");
  std::cout << spend.to_string() << "\n";

  // 2. The LEFT JOIN keeps customers with no orders, which is how you find them.
  auto idle = spend.query("orders == 0");
  std::cout << "customers who never ordered: " << idle.ncases() << "\n";
  std::cout << idle.to_string() << "\n";

  // 3. The same totals without SQL: group the orders, then read them back.
  auto per_customer = orders.group({.dropnull = true, .sort = true})
                          .by({"customer"})
                          .agg({{"orders", "order_id", DataFrame::GroupOp::Count}, {"spent", "amount", DataFrame::GroupOp::Sum}});
  std::cout << per_customer.to_string() << "\n";

  // 4. A parameterised query: the threshold is a value, not text pasted into SQL.
  DataFrame::SqlOptions big;
  big.sources.push_back({"orders", &orders});
  big.params["minimum"] = 50.0;
  auto large = customers.sql(big).execute("SELECT orders.order_id, df.customer, df.country, orders.amount FROM df JOIN orders ON orders.customer = df.customer WHERE orders.amount >= {minimum} ORDER BY orders.amount DESC");
  std::cout << large.to_string() << "\n";
}
