<!-- generated from reports_and_joins.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Reports and joins

[Back to PlussMic by example](../guide.md)

Most table work ends in a report: totals by group, the best few rows, customers matched to their orders. This page builds two such reports end to end, one from a single table and one from two tables that must be joined, with complete programs and the output they print.

## The mental model

A `DataFrame` is a set of named features (columns), each a typed `Slate`, with one row per case. Work on it is a pipeline of calls that each return a new frame, so a report is a chain: derive a feature with `assign`, filter with `query`, order with `arrange`, summarise with `group(...).by(...).agg(...)` or `aggregate`. Relational questions that involve several tables (a join, a left join to find what is missing) are easier in SQL: `frame.sql(options).execute(...)` runs a `SELECT` over the frame, which is named `df`, and over other frames you bind by name.

## A sales report

Six cities with units sold and a price. The program derives revenue, keeps the strong rows, ranks them, summarises by region and totals the table.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;

int main() {
  auto sales = DataFrame({
      {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale", "Takoradi", "Cape Coast"})},
      {"region", pluss::make_note({"south", "south", "middle", "north", "south", "middle"})},
      {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0, 9.0, 6.0})},
      {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0, 3.5, 4.5})},
  }).title("sales");
  std::cout << sales.to_string() << "\n";

  // 1. A derived feature: revenue = units x price. assign takes a function of the frame, so it can use any existing feature.
  auto enriched = sales.assign({{"revenue", [](const DataFrame &f) { return f.features("units") * f.features("price"); }}});
  std::cout << enriched.to_string() << "\n";

  // 2. Filter with a text predicate, then order the survivors by revenue, highest first.
  auto strong = enriched.query("revenue >= 28");
  std::cout << "cities with revenue of at least 28: " << strong.ncases() << "\n";
  std::cout << enriched.arrange({"revenue"}, false).head(3).to_string() << "\n";

  // 3. One row per region: total units, mean revenue and the number of cities.
  auto by_region = enriched.group({.dropnull = true, .sort = true})
                       .by({"region"})
                       .agg({{"cities", "city", DataFrame::GroupOp::Count},
                             {"units_sum", "units", DataFrame::GroupOp::Sum},
                             {"revenue_mean", "revenue", DataFrame::GroupOp::Mean}});
  std::cout << by_region.to_string() << "\n";

  // 4. The whole table in one row.
  auto total = enriched.aggregate({{"total_units", "units", DataFrame::SummaryOp::Sum}, {"total_revenue", "revenue", DataFrame::SummaryOp::Sum}}, false);
  std::cout << total.to_string() << "\n";
}
```

Output

```text
=======================================
     | city        region  units  price
---------------------------------------
0    | Accra       south   10     3    
1    | Tema        south   7      4    
2    | Kumasi      middle  12     2.5  
3    | Tamale      north   5      5    
4    | Takoradi    south   9      3.5  
5    | Cape Coast  middle  6      4.5  
---------------------------------------
[6 cs x 4 feat]
================================================
     | city        region  units  price  revenue
------------------------------------------------
0    | Accra       south   10     3      30     
1    | Tema        south   7      4      28     
2    | Kumasi      middle  12     2.5    30     
3    | Tamale      north   5      5      25     
4    | Takoradi    south   9      3.5    31.5   
5    | Cape Coast  middle  6      4.5    27     
------------------------------------------------
[6 cs x 5 feat]
cities with revenue of at least 28: 4
==============================================
     | city      region  units  price  revenue
----------------------------------------------
4    | Takoradi  south   9      3.5    31.5   
0    | Accra     south   10     3      30     
2    | Kumasi    middle  12     2.5    30     
----------------------------------------------
[3 cs x 5 feat]
==============================================
     | region  cities  units_sum  revenue_mean
----------------------------------------------
0    | middle  2       18         28.5        
1    | north   1       5          25          
2    | south   3       26         29.83333333 
----------------------------------------------
[3 cs x 4 feat]
====================================
        | total_units  total_revenue
------------------------------------
summary | 49           171.5        
------------------------------------
[1 cs x 2 feat]
```
`assign` takes a function of the frame, so `revenue` is built from whichever features it needs. `query("revenue >= 28")` keeps four of the six rows; `arrange({"revenue"}, false)` orders descending and `head(3)` keeps the leaders, where Takoradi (31.5) edges out Accra and Kumasi (both 30). The grouped table has one row per region: the south has three cities and 26 units, the middle two cities and 18 units. Each aggregation names the result feature, the source feature and the operation, so the output is exactly the columns you asked for. `aggregate` is the same idea for the whole table and gives one row, 49 units and 171.5 of revenue.

## Customers and their orders

Four customers and five orders. The program totals spending per customer with a SQL left join, finds the customer who never ordered, repeats the totals with the group API, and runs a parameterised query.

```cpp
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
```

Output

```text
=============================================
     | df.customer  df.country  orders  spent
---------------------------------------------
0    | alan         UK          1       310  
1    | ada          UK          3       182.5
2    | grace        US          1       75.5 
3    | edsger       NL          0       0    
---------------------------------------------
[4 cs x 4 feat]
customers who never ordered: 1
=============================================
     | df.customer  df.country  orders  spent
---------------------------------------------
3    | edsger       NL          0       0    
---------------------------------------------
[1 cs x 4 feat]
==============================
     | customer  orders  spent
------------------------------
0    | ada       3       182.5
1    | alan      1       310  
2    | grace     1       75.5 
------------------------------
[3 cs x 3 feat]
==============================================================
     | orders.order_id  df.customer  df.country  orders.amount
--------------------------------------------------------------
0    | 104              alan         UK          310          
1    | 101              ada          UK          120          
2    | 102              grace        US          75.5         
--------------------------------------------------------------
[3 cs x 4 feat]
```
The frame the SQL is called on is `df`, and `orders` is bound in `SqlOptions::sources`. The `LEFT JOIN` keeps every customer, so Edsger appears with zero orders and zero spent, which an inner join would have hidden; `COALESCE` turns the missing sum into zero. The second block filters that result with `query` to list the customers who never ordered. The third reaches the same totals for the customers who did order without SQL, by grouping the orders alone; note that it cannot know about Edsger, which is the reason to join first when the question includes "who is missing". The last query takes its threshold from a parameter, `{minimum}`, so a value never has to be pasted into the statement.

## Decisions

- **Pipeline or SQL:** use the pipeline for derivations, filters and group summaries on one frame; use SQL when the question is relational (joins, left joins, anti-joins) or already exists as SQL.
- **Inner or left join:** use a left join whenever "who is missing" is part of the question.
- **Sorting and limits:** `arrange` then `head` for the leaders; `ORDER BY ... LIMIT` in SQL.
- **Parameters:** pass values through `params`, identifiers through `identifier_params`, and never build SQL by concatenating user input.

## Where next

Reference pages: [`assign`](../api/dataframe/assign.md), [`query`](../api/dataframe/query.md), [`arrange`](../api/dataframe/arrange.md), [`head`](../api/dataframe/head.md), [`aggregate`](../api/dataframe/aggregate.md), [`to_string`](../api/dataframe/to_string.md). Next: [Files, missing data and time](files_and_time.md).
