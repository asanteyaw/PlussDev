<!-- generated from bundle.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Bundle: multi-table workspaces

[Back to PlussMic by example](../guide.md)

When an analysis spans several related tables, a workspace that remembers how they connect saves repeating the join logic. A `Nexus` coordinates `DataFrame`s and a `Locus` coordinates `Catalog`s. This page assembles a customer table, checks a workspace's health, walks a join path, asks SQL questions across tables, persists a workspace, and does the Locus versions.

## The mental model

A workspace holds named tables, a **declaration of what one row of each table is** (its primary key), and **relationships** between tables with a cardinality. Everything else uses those declarations. `pull(root)` assembles one row per root row with features reduced from related tables (the grain never changes). `from(root).join(...)` walks relationships and expands rows. `query()` runs SQL across the tables by name. `audit()` and its companions check the declarations against the data. The default cardinality of a declared relationship is many-to-one (left rows share a right row), so a one-to-many link must say so.

## One row per customer

Customers and their orders, with one customer who has never ordered.

```cpp
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
                       .set_table_meta("customers", customer_meta)
                       .set_table_meta("orders", order_meta)
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
```

Output

```text
===================================
     | table      ncases  nfeatures
-----------------------------------
0    | customers  4       3        
1    | orders     5       3        
-----------------------------------
[2 cs x 3 feat]
==================================================================================
     | name  country  orders_order_id_count  orders_amount_sum  orders_amount_mean
----------------------------------------------------------------------------------
0    | Ada   UK       2                      170                85                
1    | Bola  NG       2                      95                 47.5              
2    | Chen  CN       1                      33                 33                
3    | Dara  NG       0                      NULL               NULL              
----------------------------------------------------------------------------------
[4 cs x 5 feat]
customers 4, rows pulled 4
join rows: 5
```
The summary shows the two tables. `pull("customers")` returns one row per customer with the name and country copied from the root and a count, a sum and a mean reduced from the orders table: Ada has two orders totalling `170`, Bola two totalling `95`, Chen one and Dara none, with `NULL` for the sum of nothing. Four customers in, four rows out, whereas the join expands to the five order rows and loses Dara.

## Health check before analysis

Customers with a duplicated id and orders that include one for a customer who does not exist.

```cpp
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
```

Output

```text
=================================================================
     | name             right_duplicate_rows  left_orphans  valid
-----------------------------------------------------------------
0    | customers        -1                    -1            false
1    | orders           -1                    -1            true 
2    | customer_orders   2                     1            false
-----------------------------------------------------------------
[3 cs x 4 feat]
customers key valid: no
orders key valid:    yes
duplicated customer rows:
==============================
     | customer_id  name      
------------------------------
1    | 2            Bola      
2    | 2            Bola (dup)
------------------------------
[2 cs x 2 feat]
orders without a customer:
====================================
     | order_id  customer_id  amount
------------------------------------
0    | 102       9            20    
------------------------------------
[1 cs x 3 feat]
```
The audit lists one line per table and relationship: the customers table is not valid (a repeated key) and the relationship has two duplicate rows on the customer side and one orphan on the order side. `validate_keys` answers yes or no per table, `detect_duplicates` shows the two rows with customer id `2`, and `detect_orphans` shows order `102`, whose customer `9` does not exist. A check like this is the first step in any analysis of a workspace; the numbers in the answers are only as trustworthy as the keys.

## Walking a path

Orders, order lines and products: two hops from an order to its product.

```cpp
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
```

Output

```text
======================================
     | customer  category  qty   price
--------------------------------------
0    | Ada       tools     2     10   
1    | Ada       toys      1     25   
2    | Bola      tools     5     10   
3    | Ada       tools     1     40   
4    | Ada       toys      3     25   
--------------------------------------
[5 cs x 4 feat]
===============================
     | category  lines  revenue
-------------------------------
0    | tools     3      110    
1    | toys      2      100    
-------------------------------
[2 cs x 3 feat]
========================
     | customer  revenue
------------------------
0    | Ada       160    
1    | Bola      50     
------------------------
[2 cs x 2 feat]
```
`from("orders").join("lines").join("products")` follows the two declared relationships, and `pick` keeps the fields that matter. Revenue is quantity times price per line, and the totals match a hand calculation: tools `110` over three lines and toys `100` over two; Ada `160` and Bola `50`. Because the path is declared once, any question over these three tables is a few lines.

## SQL across the workspace

The same customers and orders, queried with SQL.

```cpp
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
  auto workspace = Nexus({{"customers", customers}, {"orders", orders}}).set_table_meta("customers", cm).set_table_meta("orders", om)
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
```

Output

```text
===============================
     | country  orders  revenue
-------------------------------
0    | UK       2       170    
1    | NG       2       95     
2    | CN       1       33     
-------------------------------
[3 cs x 3 feat]
==================
     | name  spent
------------------
0    | Ada   170  
1    | Bola  95   
2    | Chen  33   
3    | Dara  0    
------------------
[4 cs x 2 feat]
=====================================================
     | orders.order_id  customers.name  orders.amount
-----------------------------------------------------
0    | 103              Ada             120          
1    | 101              Bola            75           
-----------------------------------------------------
[2 cs x 3 feat]
```
Tables are addressed by name inside one statement. Revenue per country is `170` for the UK, `95` for Nigeria and `33` for China; the left join keeps Dara in the customer ranking with `0` spent; and the parameterised query takes its threshold as a value: the two orders of at least `60` come out, order `103` for Ada and order `101` for Bola.

## Saving and restoring

A workspace saved to a directory and read back.

```cpp
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
  auto workspace = Nexus({{"customers", customers}, {"orders", orders}}).set_table_meta("customers", cm).set_table_meta("orders", om)
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
```

Output

```text
before: 2 tables, 1 relationship(s)
after:  2 tables, 1 relationship(s)
pull identical after the round trip: yes
==============================
     | name  orders_amount_sum
------------------------------
0    | Ada   50               
1    | Bola  95               
2    | Chen  NULL             
------------------------------
[3 cs x 2 feat]
source of the customers table: crm export 2026-11-13
```
`save` writes the tables, the key declarations, the relationships and the saved plans; `read` restores them (`load` is a different call that scans a directory of raw table files and makes each file a table, with no relationships). The restored workspace has two tables and one relationship, the same pull gives an identical table, and the source note on the customers table comes back with it.

## Locus: the catalog version

A `Locus` is a `Nexus` for catalogs. The tables are streams, so the child streams keep their own lengths and are not forced into a rectangle.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::kindle::literals;
using pluss::bundle::Locus;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;
using pluss::relational::RelationshipCardinality;

int main() {
  Catalog customers({"customer_id"_k = Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0})), "name"_k = Stream(pluss::make_note({"Ada", "Bola", "Chen", "Dara"}))});
  Catalog orders({"order_id"_k = Stream(pluss::make_series({100.0, 101.0, 102.0, 103.0, 104.0})), "customer_id"_k = Stream(pluss::make_series({1.0, 2.0, 2.0, 1.0, 3.0})),
                  "amount"_k = Stream(pluss::make_series({50.0, 75.0, 20.0, 120.0, 33.0}))});
  pluss::relational::TableMeta cm, om;
  cm.primary_key = {"customer_id"};
  om.primary_key = {"order_id"};
  auto workspace = Locus({{"customers", customers}, {"orders", orders}})
                       .set_table_meta("customers", cm)
                       .set_table_meta("orders", om)
                       .link({.name = "customer_orders", .left_table = "customers", .right_table = "orders", .keys = {{"customer_id", "customer_id"}}, .cardinality = RelationshipCardinality::OneToMany, .right_optional = true});

  // 1. One record per customer, with a count and totals reduced from the orders catalog.
  auto features = workspace.pull("customers").take({"name"}).agg("orders").count("order_id").sum("amount").end().collect();
  std::cout << features << "\n";
  std::cout << "customers " << customers.nrows() << ", records pulled " << features.nrows() << "\n";

  // 2. The same audit as a Nexus. Dara has no orders; because the link declares the orders side optional (right_optional), that is not a violation.
  std::cout << workspace.audit_report().features({"name", "valid"}).to_string() << "\n";
}
```

Output

```text
========================================================================================================================
kindle::Catalog  keys=3
keys: [name, orders_order_id_count, orders_amount_sum]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
name                     | Note         | Note             | 4       | [Ada, Bola, Chen, Dara]                          
orders_order_id_count    | Series       | Series[int64]    | 4       | [2, 2, 1, 0]                                     
orders_amount_sum        | Series       | Series[float64]  | 4       | [170, 95, 33, NA]                                
========================================================================================================================

customers 4, records pulled 4
=============================
     | name             valid
-----------------------------
0    | customers        true 
1    | orders           true 
2    | customer_orders  true 
-----------------------------
[3 cs x 2 feat]
```
The pull gives the same customer table as the Nexus: counts `2`, `2`, `1`, `0` and sums `170`, `95`, `33` and `NA`. The audit is the same too, and it is valid because the link declares the orders side optional: a customer without orders is a fact about the data and not a violation.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::kindle::literals;
using pluss::bundle::Locus;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;
using pluss::relational::RelationshipCardinality;

int main() {
  Catalog shipments({"shipment"_k = Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})), "port"_k = Stream(pluss::make_note({"A", "B", "A", "C", "Z"})), "tonnes"_k = Stream(pluss::make_series({12.0, 7.5, 20.0, 3.0, 9.0}))});
  Catalog ports({"port"_k = Stream(pluss::make_note({"A", "B", "C", "D"})), "country"_k = Stream(pluss::make_note({"GH", "NG", "SN", "CI"}))});
  pluss::relational::TableMeta sm, pm;
  sm.primary_key = {"shipment"};
  pm.primary_key = {"port"};
  auto workspace = Locus({{"shipments", shipments}, {"ports", ports}})
                       .set_table_meta("shipments", sm)
                       .set_table_meta("ports", pm)
                       .link({.name = "shipment_port", .left_table = "shipments", .right_table = "ports", .keys = {{"port", "port"}}, .cardinality = RelationshipCardinality::ManyToOne});

  // 1. Join shipments to their port's country. Shipment 5 goes to port Z, which is not in the ports table.
  auto detailed = workspace.join_detailed("shipments", "ports");
  std::cout << detailed.catalog << "\n";

  // 2. The report says exactly what happened: nothing is lost uncounted.
  std::cout << "report: " << detailed.report.summary() << "\n";

  // 3. The orphans on each side, from the relationship audit: shipments to unknown ports, and ports no shipment uses.
  std::cout << workspace.relationship_audit("shipment_port").features({"left_orphans", "right_orphans", "valid"}).to_string() << "\n";
}
```

Output

```text
========================================================================================================================
kindle::Catalog  keys=4
keys: [shipment, port, tonnes, country]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
shipment                 | Series       | Series[float64]  | 4       | [1, 2, 3, 4]                                     
port                     | Note         | Note             | 4       | [A, B, A, C]                                     
tonnes                   | Series       | Series[float64]  | 4       | [12, 7.5, 20, 3]                                 
country                  | Note         | Note             | 4       | [GH, NG, GH, SN]                                 
========================================================================================================================

report: 4 output record(s); 4 matched pair(s). left: 1 of 5 record(s) without a partner (0 without a key), 1 omitted. right: 1 of 4 without a partner (0 without a key), 1 omitted.
=========================================
     | left_orphans  right_orphans  valid
-----------------------------------------
0    | 1             1              false
-----------------------------------------
[1 cs x 3 feat]
```
Shipments are joined to their ports. Shipment 5 goes to port Z, which is not in the ports table, and port D has no shipment. The join keeps four records, and the report says so exactly: four output records, four matched pairs, one shipment without a partner and one port without a partner, each omitted. The relationship audit shows one orphan on each side, so the declared link is not valid until the data is repaired.

## Decisions

- **Nexus or Locus:** a Nexus for tables (`DataFrame`), a Locus for catalogs when stream semantics and ragged data matter.
- **Declare first:** primary keys and cardinalities are declarations; the checks compare them with the data.
- **Pull or join:** `pull` to keep one row per root row, a join to expand.
- **Persist with `save` and `read`:** `load` is for a directory of raw files.

## Where next

Reference pages: [`pull`](../api/nexus/pull.md), [`audit`](../api/nexus/audit.md), [`validate_keys`](../api/nexus/validate_keys.md), [`detect_orphans`](../api/nexus/detect_orphans.md), [`detect_duplicates`](../api/nexus/detect_duplicates.md), [`from`](../api/nexus/from.md), [`query`](../api/nexus/query.md), [`save`](../api/nexus/save.md), [`read`](../api/nexus/read.md), [`set_table_meta`](../api/nexus/set_table_meta.md), [`add_relationship`](../api/nexus/add_relationship.md), [`pull`](../api/locus/pull.md), [`audit_report`](../api/locus/audit_report.md), [`join`](../api/locus/join.md), [`link`](../api/locus/link.md), [`relationship_audit`](../api/locus/relationship_audit.md). Back to [PlussMic by example](../guide.md).
