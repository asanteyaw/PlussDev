<!-- generated from end_to_end_workflows.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Three complete workflows

[Back to PlussMic by example](../guide.md)

These are the jobs people actually bring to a table library: checking an export before it goes anywhere, measuring whether customers come back, and forecasting with a backtest you can believe. Each is one complete program with the output it printed.

## Auditing an export before you load it

A customer export has a duplicated id, a malformed email, an empty email, a missing age and two impossible ages. The program counts the holes, finds the duplicate keys, tests the business rules, and splits the table into accepted rows and rejected rows with a reason for each rejection.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;

int main() {
  const double none = std::numeric_limits<double>::quiet_NaN();
  auto export_table = DataFrame({
      {"id", pluss::make_series({1, 2, 3, 3, 4, 5, 6, 7})},
      {"email", pluss::make_note({"ada@example.com", "grace@navy.mil", "alan@@bad", "alan@example.org", "edsger@dijkstra.nl", "", "barbara@mit.edu", "ken@bell-labs.com"})},
      {"age", pluss::make_series({36.0, 85.0, none, 41.0, 200.0, 29.0, 34.0, 17.0})},
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
```

Output

```text
==============================================
     | id    email               age   country
----------------------------------------------
0    | 1     ada@example.com     36    UK     
1    | 2     grace@navy.mil      85    US     
2    | 3     alan@@bad           NULL  UK     
3    | 3     alan@example.org    41    UK     
4    | 4     edsger@dijkstra.nl  200   NL     
5    | 5                         29    US     
6    | 6     barbara@mit.edu     34    US     
7    | 7     ken@bell-labs.com   17    US     
----------------------------------------------
[8 cs x 4 feat]
missing values per feature:
====================================
        | id    email  age   country
------------------------------------
summary | 0     0      1     0      
------------------------------------
[1 cs x 4 feat]
ids that appear more than once:
=================
     | id    rows
-----------------
2    | 3     2   
-----------------
[1 cs x 2 feat]
rows with a valid age: 5 of 8
rows with a plausible email: 6
accepted:
===========================================
     | id    email            age   country
-------------------------------------------
0    | 1     ada@example.com  36    UK     
1    | 2     grace@navy.mil   85    US     
2    | 6     barbara@mit.edu  34    US     
-------------------------------------------
[3 cs x 4 feat]
rejected:
=======================================================
     | id    email               age   reason          
-------------------------------------------------------
0    | 3     alan@@bad           NULL  age missing     
1    | 3     alan@example.org    41    duplicate id    
2    | 4     edsger@dijkstra.nl  200   age out of range
3    | 5                         29    bad email       
4    | 7     ken@bell-labs.com   17    age out of range
-------------------------------------------------------
[5 cs x 4 feat]
accepted 3 + rejected 5 rows from 8
```
Only one value is `NULL` (an age); the empty email is an empty string, not a null, so the missing-value count does not see it and the email rule has to. That is the point of testing rules instead of trusting counts. The id `3` appears twice. Five rows have a valid age and six a plausible email. The final split is judged on the whole export: id `3` is rejected on both of its rows, once because its age is missing and once as a duplicate, because uniqueness is decided on the full table and not on the rows that survive the other rules. The accepted three and the rejected five add up to the eight rows that came in, which is the check that nothing was lost between the two tables.

## Cohort retention

Ten customers, grouped by the month they signed up. The program counts the distinct customers active at each age, divides by the size of the cohort, and lays the rates out as a cohort table.

```cpp
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
```

Output

```text
=====================================
     | cohort  age   active_customers
-------------------------------------
0    | 1       0     3               
1    | 1       1     3               
2    | 1       2     1               
3    | 1       3     2               
4    | 2       0     3               
5    | 2       1     3               
6    | 2       2     1               
7    | 3       0     3               
8    | 3       1     1               
9    | 4       0     1               
-------------------------------------
[10 cs x 3 feat]
======================================
     | cohort  age   active  retention
--------------------------------------
0    | 1       0     3       1        
1    | 1       1     3       1        
2    | 1       2     1       0.333    
3    | 1       3     2       0.667    
4    | 2       0     3       1        
5    | 2       1     3       1        
6    | 2       2     1       0.333    
7    | 3       0     3       1        
8    | 3       1     1       0.333    
9    | 4       0     1       1        
--------------------------------------
[10 cs x 4 feat]
================================
     | 0     1      2      3    
--------------------------------
1    | 1     1      0.333  0.667
2    | 1     1      0.333  NULL 
3    | 1     0.333  NULL   NULL 
4    | 1     NULL   NULL   NULL 
--------------------------------
[4 cs x 4 feat]
retention one month after signup:
======================================
     | cohort  age   active  retention
--------------------------------------
1    | 1       1     3       1        
5    | 2       1     3       1        
8    | 3       1     1       0.333    
--------------------------------------
[3 cs x 4 feat]
```
The first table counts distinct active customers by cohort and age (months since signup). Age zero is the cohort size, so dividing by it gives retention: the first cohort keeps all three customers at age one, one customer at age two and two at age three, so its retention is not monotone, a customer who lapses can return. The pivot table has one row per cohort and one column per age, and its `NULL` cells are ages that have not happened yet (the fourth cohort has only its first month), which is different from zero retention. The last query reads the number most teams watch, retention one month after signup: `1`, `1` and `0.333`, which says the latest cohort is the one to look at.

## A forecast you can trust

Three years of monthly demand with a trend and a yearly cycle. The program builds lag features, splits by time, fits a regression on the past and compares it with the rule "same month last year" on the future.

```cpp
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>

using pluss::ScalarType;
using pluss::table::DataFrame;

int main() {
  // Three years of monthly demand: a rising trend, a yearly cycle and noise.
  const std::int64_t months = 36;
  auto t = pluss::series::arange(0, months, ScalarType::Float64);
  auto demand = t.mul(2.0).add(200.0).add(t.mul(2.0 * 3.14159265358979 / 12.0).sin().mul(30.0)).add(pluss::series::randn({months}, ScalarType::Float64, 12).mul(5.0));
  auto frame = DataFrame({{"month", pluss::series::arange(0, months, ScalarType::Int64)}, {"demand", demand}});

  // 1. Lag features: demand one month ago and twelve months ago. The first twelve rows have no twelve-month lag, so they are cut.
  auto with_lags = frame.assign({{"lag1", [](const DataFrame &f) { return f.features("demand").ts().lag(1); }},
                                 {"lag12", [](const DataFrame &f) { return f.features("demand").ts().lag(12); }}});
  auto usable = with_lags.slice(12, months);
  std::cout << "usable rows " << usable.ncases() << " (of " << months << "), features " << usable.nfeatures() << "\n";

  // 2. Split by time: train on months 12-23, test on months 24-35. Shuffling would leak the future into the training set.
  auto train = usable.slice(0, 12);
  auto test = usable.slice(12, 24);
  std::cout << "train months " << train.features("month").as_series().min().item<double>() << " to " << train.features("month").as_series().max().item<double>()
            << ", test months " << test.features("month").as_series().min().item<double>() << " to " << test.features("month").as_series().max().item<double>() << "\n";

  // 3. Fit demand ~ 1 + lag1 + lag12 on the training rows by least squares.
  const auto design = [](const DataFrame &d) {
    auto ones = pluss::series::ones({d.ncases(), 1}, ScalarType::Float64);
    return ones.unite({d.features("lag1").as_series().unsqueeze(1), d.features("lag12").as_series().unsqueeze(1)}, 1);
  };
  auto x = design(train);
  auto y = train.features("demand").as_series().unsqueeze(1);
  auto beta = x.t().matmul(x).linearalg().inverse().matmul(x.t().matmul(y));
  std::cout << "coefficients (intercept, lag1, lag12): " << pluss::series::print(beta.squeeze(1)) << "\n";

  // 4. Forecast the test months two ways and score both: mean absolute error and mean absolute percentage error.
  auto actual = test.features("demand").as_series();
  auto regression = design(test).matmul(beta).squeeze(1);
  auto seasonal_naive = test.features("lag12").as_series();
  const auto score = [&](const char *name, const pluss::Series &forecast) {
    auto error = forecast.sub(actual).abs();
    std::cout << name << ": MAE " << error.mean().item<double>() << ", MAPE " << 100.0 * error.div(actual).mean().item<double>() << "%\n";
  };
  score("same month last year", seasonal_naive);
  score("regression on lag1, lag12", regression);
}
```

Output

```text
usable rows 24 (of 36), features 4
train months 12 to 23, test months 24 to 35
coefficients (intercept, lag1, lag12): Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[11.9114, 0.0910, 0.9541]
same month last year: MAE 22.3993, MAPE 8.73939%
regression on lag1, lag12: MAE 4.90166, MAPE 1.92328%
```
The lags cost the first twelve rows, leaving twenty-four: twelve to train on (months 12 to 23) and twelve to test (months 24 to 35). The split is by time because a random split would train on months that come after the ones it is scored on, flattering any model. The fitted lag-12 coefficient is `0.95` and the lag-1 coefficient `0.09`; the intercept `11.9` lifts the forecast to follow the trend. "Same month last year" ignores that demand rises by about 24 a year, so it is wrong by `22.4` on average (`8.7 %`); the regression is wrong by `4.9` (`1.9 %`). Notice that the program slices the lagged frame before it reads numbers: the lag of an early row does not exist, and reading it through `as_series()` would see a zero instead of a missing value (see [Files, missing data and time](files_and_time.md)).

## Decisions

- **Audit before you trust:** count missing values, test the rules you care about (an empty string is not a null), and reconcile accepted plus rejected against the input.
- **Cohorts:** count distinct customers, divide by the cohort size, and keep unobserved cells empty rather than zero.
- **Backtests:** split by time, build features from the past only, and compare with a simple rule before believing a model.
- **Judge on the whole table:** uniqueness and totals are properties of the full export; compute them before filtering.

## Where next

Reference pages: [`isnull`](../api/dataframe/isnull.md), [`group`](../api/dataframe/group.md), [`pivot_table`](../api/dataframe/pivot_table.md), [`sql`](../api/dataframe/sql.md), [`slice`](../api/dataframe/slice.md), [`assign`](../api/dataframe/assign.md), [`ts`](../api/dataframe/ts.md), and the [time-series accessor](../api/dataframe-ts/lag.md). Back to [PlussMic by example](../guide.md).
