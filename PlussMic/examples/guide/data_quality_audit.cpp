// Audit a customer export before it is loaded anywhere: count the holes, find duplicates, test business rules,
// and split the table into rows that pass and rows that are rejected, with the reason for each rejection.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;
using pluss::table::Shelf;

int main() {
  auto export_table = DataFrame({
      {"id", pluss::make_series({1, 2, 3, 3, 4, 5, 6, 7})},
      {"email", pluss::make_note({"ada@example.com", "grace@navy.mil", "alan@@bad", "alan@example.org", "edsger@dijkstra.nl", "", "barbara@mit.edu", "ken@bell-labs.com"})},
      // The third customer's age was never recorded: the validity mask marks it missing (the 0.0 under it is only a placeholder).
      {"age", Shelf(pluss::make_series({36.0, 85.0, 0.0, 41.0, 200.0, 29.0, 34.0, 17.0}),
                    pluss::make_series(std::vector<bool>{true, true, false, true, true, true, true, true}))},
      {"country", pluss::make_note({"UK", "US", "UK", "UK", "NL", "US", "US", "US"})},
  });
  std::cout << export_table.to_string() << "\n";

  // 1. How many cells are missing in each feature? isnull gives a Boolean frame; summing it counts.
  auto missing = export_table.isnull();
  std::cout << "missing values per feature:\n" << missing.aggregate({{"id", "id", DataFrame::SummaryOp::Sum}, {"email", "email", DataFrame::SummaryOp::Sum}, {"age", "age", DataFrame::SummaryOp::Sum}, {"country", "country", DataFrame::SummaryOp::Sum}}, false).to_string() << "\n";

  // 2. Duplicate keys: an id that appears twice makes joins ambiguous. Count the occurrences of each id.
  auto per_id = export_table.group({.dropnull = true, .sort = true}).by({"id"}).agg({{"rows", "id", DataFrame::GroupOp::Count}});
  std::cout << "ids that appear more than once:\n" << per_id.query("rows > 1").to_string() << "\n";

  // 3. Business rules, each as a query: a plausible age and an email with exactly one '@'.
  auto age_ok = export_table.query("age >= 18 and age <= 100");
  std::cout << "rows with a valid age: " << age_ok.ncases() << " of " << export_table.ncases() << "\n";
  auto email_ok = export_table.sql().execute("SELECT id FROM df WHERE email LIKE '%_@_%.__%' AND email NOT LIKE '%@%@%'");
  std::cout << "rows with a plausible email: " << email_ok.ncases() << "\n";

  // 4. Accepted = every rule passes and the id is unique in the whole export; rejected = everything else, with a reason.
  //    Uniqueness is judged on the full table, not on the rows that pass the other rules.
  auto accepted = export_table.sql().execute(
      "SELECT id, email, age, country FROM df "
      "WHERE age BETWEEN 18 AND 100 AND email LIKE '%_@_%.__%' AND email NOT LIKE '%@%@%' "
      "AND id NOT IN (SELECT id FROM df GROUP BY id HAVING COUNT(*) > 1) ORDER BY id");
  std::cout << "accepted:\n" << accepted.to_string() << "\n";
  auto rejected = export_table.sql().execute(
      "SELECT id, email, age, "
      "CASE WHEN age IS NULL THEN 'age missing' "
      "WHEN age < 18 OR age > 100 THEN 'age out of range' "
      "WHEN email NOT LIKE '%_@_%.__%' OR email LIKE '%@%@%' THEN 'bad email' "
      "ELSE 'duplicate id' END AS reason "
      "FROM df WHERE age IS NULL OR age < 18 OR age > 100 OR email NOT LIKE '%_@_%.__%' OR email LIKE '%@%@%' OR id IN (SELECT id FROM df GROUP BY id HAVING COUNT(*) > 1) "
      "ORDER BY id");
  std::cout << "rejected:\n" << rejected.to_string() << "\n";
  std::cout << "accepted " << accepted.ncases() << " + rejected " << rejected.ncases() << " rows from " << export_table.ncases() << "\n";
}
