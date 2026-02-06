# Feed-First Design Guide

## Overview

The **Handle** module follows a **Feed-First design philosophy**, mirroring the Thread-First design of the Table module but for MongoDB-like NoSQL operations.

### Design Philosophy

```
Feed (1D)      →  Catalog (2D)  →  Archive (3D)
Document Field    Collection       Database

MongoDB-like      NoSQL            Schema-free
Flexible types    CRUD operations  Cross-collection queries
```

## Architecture Comparison

| Layer | Table Module (SQL-like) | Handle Module (MongoDB-like) |
|-------|-------------------------|------------------------------|
| **1D** | Thread (column) | Feed (document field) |
| **2D** | DataFrame (table) | Catalog (collection) |
| **3D** | Atlas (database) | Archive (database) |
| **Design** | Thread-First | Feed-First |
| **Paradigm** | Columnar/Relational | Document/NoSQL |
| **Operations** | SQL-inspired | MongoDB-inspired |

## Feed (1D) - Document Field

### Core Concept
A **Feed** is a flexible document field that can hold:
- Scalar values: `null`, `bool`, `int64_t`, `double`, `string`, `Chord`
- Arrays: vectors, `Series` (numerical), `Note` (text)
- Nested documents: key-value pairs of Feeds

### MongoDB-Inspired Operations

#### Value Operations
```cpp
auto field = Feed(42.0, "price");

// $set: Set new value
field = field.set(Feed::ScalarValue(50.0));

// $inc: Increment
field = field.inc(5.0);  // price = 47.0

// $mul: Multiply
field = field.mul(1.1);  // price *= 1.1

// $min/$max: Update if condition met
field = field.min(Feed::ScalarValue(40.0));  // Take minimum
field = field.max(Feed::ScalarValue(60.0));  // Take maximum
```

#### Array Operations
```cpp
auto tags = Feed(std::vector<std::string>{"electronics", "sale"}, "tags");

// $push: Append element
tags = tags.push(Feed::ScalarValue(std::string("new")));

// $pull: Remove matching elements
tags = tags.pull(Feed::ScalarValue(std::string("sale")));

// $addToSet: Add if not exists (unique)
tags = tags.add_to_set(Feed::ScalarValue(std::string("featured")));

// $pop: Remove first or last
tags = tags.pop(1);  // Remove last element
tags = tags.pop(-1); // Remove first element
```

#### Document Operations
```cpp
Feed::DocumentValue nested_doc;
nested_doc["city"] = std::make_shared<Feed>("Seattle", "city");
nested_doc["zip"] = std::make_shared<Feed>(98101, "zip");

auto address = Feed(nested_doc, "address");

// Access nested fields
auto city = address.get_field("city");

// Set nested fields
address = address.set_field("country", Feed("USA", "country"));

// Remove fields
address = address.unset_field("zip");

// Check field existence
bool has_city = address.has_field("city");
```

#### String Operations
```cpp
auto name = Feed("john doe", "name");

name = name.to_upper();           // "JOHN DOE"
name = name.to_lower();           // "john doe"
name = name.trim();               // Remove whitespace
name = name.concat(" Jr.");       // "john doe Jr."

bool contains = name.contains("doe");
bool starts = name.starts_with("john");
bool ends = name.ends_with("doe");
```

#### Numeric Operations
```cpp
auto value = Feed(-42.7, "value");

value = value.abs();              // 42.7
value = value.round(1);           // 42.7
value = value.floor();            // 42.0
value = value.ceil();             // 43.0
value = value.sqrt();             // √42 ≈ 6.48
value = value.pow(2.0);           // 42²
```

#### Comparison Operations (for Queries)
```cpp
auto price = Feed(50.0, "price");
auto threshold = Feed(100.0, "threshold");

bool eq = price.eq(threshold);    // $eq: equals
bool ne = price.ne(threshold);    // $ne: not equals
bool lt = price.lt(threshold);    // $lt: less than
bool le = price.le(threshold);    // $lte: less than or equal
bool gt = price.gt(threshold);    // $gt: greater than
bool ge = price.ge(threshold);    // $gte: greater than or equal

// $in: value in list
bool in_range = price.in({
    Feed::ScalarValue(40.0),
    Feed::ScalarValue(50.0),
    Feed::ScalarValue(60.0)
});
```

## Catalog (2D) - Document Collection

### Core Concept
A **Catalog** is a collection of documents (like MongoDB collection). Each document is represented as an `unordered_map<string, Feed>`.

### CRUD Operations

#### Insert
```cpp
using namespace pluss::handle;

auto catalog = std::make_shared<Catalog>("products");

// Insert single document
Catalog::Document product1 = {
    {"name", Feed("Laptop", "name")},
    {"price", Feed(999.99, "price")},
    {"stock", Feed(50, "stock")},
    {"tags", Feed(std::vector<std::string>{"electronics", "computers"}, "tags")}
};

catalog = catalog->insert_one(product1);

// Insert multiple documents
std::vector<Catalog::Document> products = {product1, product2, product3};
catalog = catalog->insert_many(products);
```

#### Find (Query)
```cpp
// MongoDB-style query builders
auto query = Catalog::eq("price", Feed::ScalarValue(999.99));
auto results = catalog->find(query);

// Complex queries with logical operators
auto complex_query = Catalog::and_({
    Catalog::gte("price", Feed::ScalarValue(100.0)),
    Catalog::lte("price", Feed::ScalarValue(1000.0)),
    Catalog::in("category", {
        Feed::ScalarValue(std::string("electronics")),
        Feed::ScalarValue(std::string("computers"))
    })
});

auto filtered = catalog->find(complex_query);

// Find with projection (select specific fields)
Catalog::Projection fields = {"name", "price"};
auto projected = catalog->find(query, fields);

// Find one document
auto doc = catalog->find_one(query);

// Count matching documents
int64_t count = catalog->count(query);

// Check existence
bool exists = catalog->exists(query);
```

#### Update
```cpp
// Update single document
auto update_op = Catalog::set("price", Feed(899.99, "price"));
catalog = catalog->update_one(query, update_op);

// Update many documents
auto bulk_update = Catalog::inc("stock", 10.0);
catalog = catalog->update_many(query, bulk_update);

// Multiple update operations
auto multi_update = [](const Catalog::Document& doc) {
    auto updated = doc;
    updated["price"] = doc.at("price").inc(-50.0);  // Decrease price
    updated["sale"] = Feed(true, "sale");           // Add sale flag
    return updated;
};
catalog = catalog->update_many(query, multi_update);

// Replace entire document
catalog = catalog->replace_one(query, new_document);
```

#### Delete
```cpp
// Delete single document
catalog = catalog->delete_one(query);

// Delete many documents
auto del_query = Catalog::lt("stock", Feed::ScalarValue(10));
catalog = catalog->delete_many(del_query);
```

### Aggregation Operations

#### Simple Aggregations
```cpp
// Group by field
auto grouped = catalog->group("category");

// Match (filter)
auto matched = catalog->match(Catalog::gte("price", Feed::ScalarValue(100.0)));

// Project (select fields)
auto projected = catalog->project({"name", "price", "category"});

// Sort
auto sorted = catalog->sort("price", true);  // ascending

// Limit and skip
auto limited = catalog->limit(10);
auto paginated = catalog->skip(20)->limit(10);
```

#### Aggregation Pipeline
```cpp
// Complex aggregation pipeline
auto results = catalog->aggregate()
    ->match(Catalog::gte("price", Feed::ScalarValue(100.0)))
    ->group("category")
    ->project({"category", "avg_price", "count"})
    ->sort("avg_price", false)  // descending
    ->limit(5)
    ->execute();
```

#### Statistical Aggregations (Feed-First)
```cpp
// Delegate to Feed operations
double total_revenue = catalog->sum("price");
double avg_price = catalog->avg("price");
double min_price = catalog->min("price");
double max_price = catalog->max("price");
int64_t num_products = catalog->count_field("name");
```

### Indexing

```cpp
// Create index for faster queries
catalog->create_index("email", true);  // unique index
catalog->create_index("category", false);

// Drop index
catalog->drop_index("category");

// List all indexes
auto indexes = catalog->list_indexes();
```

## Archive (3D) - Database

### Core Concept
An **Archive** is a database containing multiple Catalogs (collections). It provides database-level operations and cross-collection queries.

### Collection Management

```cpp
using namespace pluss::handle;

auto archive = std::make_shared<Archive>("ecommerce_db");

// Create collections
archive = archive->create_collection("products");
archive = archive->create_collection("customers");
archive = archive->create_collection("orders");

// Access collection
auto products = archive->get_collection("products");

// List collections
auto collections = archive->list_collections();
// ["products", "customers", "orders"]

// Drop collection
archive = archive->drop_collection("old_collection");

// Rename collection
archive = archive->rename_collection("old_name", "new_name");
```

### Cross-Collection Operations

```cpp
// Union: Combine multiple collections
auto combined = archive->union_collections({"collection1", "collection2"});

// Intersect: Find common documents
auto common = archive->intersect_collections(
    {"users", "active_users"},
    "user_id"  // field to match on
);

// Merge archives
auto merged = archive->merge_archives(other_archive);
```

### Database-Level Aggregations

```cpp
// Aggregate across all collections
double total_revenue = archive->sum_across("revenue");
double avg_price = archive->avg_across("price");
int64_t total_docs = archive->count_across();

// Collection statistics
auto sizes = archive->collection_sizes();
// {"products": 1000, "customers": 5000, "orders": 3000}

int64_t product_count = archive->count_documents("products");
double total_size = archive->total_size_mb();
```

### Search Across Collections

```cpp
// Find in all collections
auto query = Catalog::eq("status", Feed::ScalarValue(std::string("active")));
auto results = archive->find_in_all(query);
// Returns: vector<pair<collection_name, vector<Document>>>

// Count in all collections
auto counts = archive->count_in_all(query);
// {"products": 50, "customers": 200, "orders": 150}
```

### Map/Filter Operations (Feed-First)

```cpp
// Apply operation to all collections
auto updated = archive->map_collections([](const auto& catalog) {
    // Add timestamp to all documents
    return catalog->update_many(
        [](const auto&) { return true; },  // all documents
        Catalog::current_date("updated_at")
    );
});

// Filter collections by condition
auto large_collections = archive->filter_collections([](const auto& catalog) {
    return catalog->size() > 1000;
});
```

### Transactions

```cpp
// Begin transaction
auto txn = archive->begin_transaction();

// Add operations
txn->insert("orders", order_doc);
txn->update("products", query, update_stock);
txn->delete_doc("cart", cart_query);

// Commit or rollback
auto updated_archive = txn->commit();
// OR
txn->rollback();
```

### Backup/Restore

```cpp
// Export entire database
std::string json = archive->export_to_json();
std::string bson = archive->export_to_bson();

// Save to file
archive->save_to_file("backup.json", "json");
archive->save_to_file("backup.bson", "bson");

// Import from file
auto restored = Archive::load_from_file("backup.json", "json");

// Snapshot/restore
auto snapshot = archive->snapshot();
archive = archive->restore(snapshot);
```

## Feed-First vs Thread-First

### Design Philosophy Comparison

**Thread-First (Table - SQL-like)**:
```cpp
// Columnar operations
auto df = make_shared<DataFrame>(data);

// Operations on columns (Threads)
auto mean_col = df->get_col("price").mean();     // Thread operation
auto df_filtered = df->loc(condition);            // Delegate to Thread.loc()
auto df_sorted = df->sort_values("price");        // Use Thread.sort_values()
auto grouped = df->groupby("category")->sum();    // Thread aggregations

// DataFrame delegates to Thread operations
// Each column is a Thread that knows how to process itself
```

**Feed-First (Handle - MongoDB-like)**:
```cpp
// Document operations
auto catalog = make_shared<Catalog>("products");

// Operations on documents (Feeds)
auto doc = catalog->find_one(query);              // Document retrieval
auto price_field = doc.at("price");               // Feed extraction
auto updated_price = price_field.inc(10.0);       // Feed operation

// Catalog delegates to Feed operations
// Each field is a Feed that knows how to transform itself
```

### Operational Paradigms

#### Thread-First (Columnar)
```
Thread operations → DataFrame operations → Atlas operations
    (1D)                 (2D)                   (3D)

mean() on column → mean() on all columns → mean() across tables
```

#### Feed-First (Document)
```
Feed operations → Catalog operations → Archive operations
    (1D)              (2D)                  (3D)

inc() on field → update() documents → update() across collections
```

## MongoDB Operator Reference

### Query Operators
| Operator | PlussMa Method | Description |
|----------|----------------|-------------|
| `$eq` | `Catalog::eq()` | Equals |
| `$ne` | `Catalog::ne()` | Not equals |
| `$lt` | `Catalog::lt()` | Less than |
| `$lte` | `Catalog::lte()` | Less than or equal |
| `$gt` | `Catalog::gt()` | Greater than |
| `$gte` | `Catalog::gte()` | Greater than or equal |
| `$in` | `Catalog::in()` | In array |
| `$nin` | `Catalog::nin()` | Not in array |
| `$exists` | `Catalog::exists()` | Field exists |
| `$type` | `Catalog::type()` | Field type |
| `$regex` | `Catalog::regex()` | Regular expression |
| `$and` | `Catalog::and_()` | Logical AND |
| `$or` | `Catalog::or_()` | Logical OR |
| `$not` | `Catalog::not_()` | Logical NOT |
| `$nor` | `Catalog::nor()` | Logical NOR |

### Update Operators
| Operator | PlussMa Method | Description |
|----------|----------------|-------------|
| `$set` | `Feed::set()` or `Catalog::set()` | Set field value |
| `$unset` | `Catalog::unset()` | Remove field |
| `$inc` | `Feed::inc()` or `Catalog::inc()` | Increment |
| `$mul` | `Feed::mul()` or `Catalog::mul()` | Multiply |
| `$rename` | `Feed::rename()` or `Catalog::rename()` | Rename field |
| `$min` | `Feed::min()` | Update if less |
| `$max` | `Feed::max()` | Update if greater |
| `$currentDate` | `Catalog::current_date()` | Set to current date |
| `$push` | `Feed::push()` | Append to array |
| `$pull` | `Feed::pull()` | Remove from array |
| `$pop` | `Feed::pop()` | Remove first/last |
| `$addToSet` | `Feed::add_to_set()` | Add unique to array |

## Next Steps

1. **Implementation**: Implement core Feed, Catalog, and Archive operations
2. **Testing**: Create comprehensive test suite
3. **Examples**: Build real-world examples (user management, e-commerce, logging)
4. **Optimization**: Add indexing, caching, and query optimization
5. **Integration**: Connect with MongoDB for persistence
6. **Documentation**: API reference and tutorials

## Summary

**Feed-First Design** provides a flexible, MongoDB-inspired NoSQL interface within PlussMa:

- **Feed (1D)**: Flexible document fields with rich operations
- **Catalog (2D)**: Document collections with CRUD and aggregation
- **Archive (3D)**: Databases with cross-collection queries

This complements the **Thread-First Design** (SQL-like) to provide both relational and document-oriented data manipulation in a unified framework.
