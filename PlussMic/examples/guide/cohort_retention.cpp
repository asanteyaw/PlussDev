// Do customers come back? Group customers by the month they signed up, count how many are still active k months later,
// and lay the retention rates out as a cohort table.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;

int main() {
  // One row per (customer, month in which the customer was active). Months are numbered 1 to 4.
  auto activity = DataFrame({
      {"customer", pluss::make_series({1, 1, 1, 1, 2, 2, 3, 3, 3, 4, 4, 5, 5, 5, 6, 6, 7, 8, 8, 9, 10, 10})},
      {"signup", pluss::make_series({1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4})},
      {"active", pluss::make_series({1, 2, 3, 4, 1, 2, 1, 2, 4, 2, 3, 2, 3, 4, 2, 3, 3, 3, 4, 3, 4, 4})},
  });

  // 1. For each signup month (cohort) and each age (months since signup), count distinct active customers.
  auto counts = activity.sql().execute(
      "SELECT signup AS cohort, active - signup AS age, COUNT(DISTINCT customer) AS active_customers "
      "FROM df GROUP BY signup, active - signup ORDER BY cohort, age");
  std::cout << counts.to_string() << "\n";

  // 2. Cohort size is the count at age 0. Divide each count by it to get a retention rate.
  DataFrame::SqlOptions options;
  options.sources.push_back({"counts", &counts});
  auto rates = counts.sql(options).execute(
      "SELECT c.cohort AS cohort, c.age AS age, c.active_customers AS active, "
      "ROUND(1.0 * c.active_customers / s.active_customers, 3) AS retention "
      "FROM df AS c JOIN counts AS s ON s.cohort = c.cohort AND s.age = 0 ORDER BY c.cohort, c.age");
  std::cout << rates.to_string() << "\n";

  // 3. The cohort table: one row per cohort, one column per age.
  auto table = rates.pivot_table("cohort", "age", "retention", DataFrame::GroupOp::Mean, true, false);
  std::cout << table.to_string() << "\n";

  // 4. Month-1 retention across cohorts: the number to watch.
  auto month1 = rates.query("age == 1");
  std::cout << "retention one month after signup:\n" << month1.to_string() << "\n";
}
