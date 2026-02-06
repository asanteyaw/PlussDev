/**
 * @file feed_first_example.cpp
 * @brief Comprehensive demonstration of Feed-First design philosophy
 * 
 * This example showcases the Feed → Catalog → Archive hierarchy,
 * mirroring the Thread → DataFrame → Atlas design but for MongoDB-like operations.
 */

#include <iostream>
#include <pluss/pluss.h>
#include <pluss/datalog/handle/feed/feed.h>
#include <pluss/datalog/handle/catalog/catalog.h>
#include <pluss/datalog/handle/archive/archive.h>

using namespace pluss;
using namespace pluss::handle;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(70, '=') << "\n\n";
}

int main() {
    print_separator("Feed-First Design Philosophy");
    
    std::cout << "Architecture:\n";
    std::cout << "  Feed (1D)    → Document field operations\n";
    std::cout << "  Catalog (2D) → Collection of documents (MongoDB-like)\n";
    std::cout << "  Archive (3D) → Database of collections\n\n";
    
    // ============================================================
    // Example 1: Feed (1D) - Document Field Operations
    // ============================================================
    print_separator("1. Feed (1D) - Document Field Operations");
    
    std::cout << "Creating Feeds (document fields):\n\n";
    
    // Scalar values
    auto name = Feed("John Doe", "name");
    auto age = Feed(30, "age");
    auto price = Feed(99.99, "price");
    auto is_active = Feed(true, "is_active");
    
    std::cout << "Scalar Feeds:\n";
    std::cout << "  name: " << name.to_string() << "\n";
    std::cout << "  age: " << age.to_string() << "\n";
    std::cout << "  price: " << price.to_string() << "\n";
    std::cout << "  is_active: " << is_active.to_string() << "\n\n";
    
    // Array values
    std::vector<std::string> tags_vec = {"electronics", "sale", "featured"};
    auto tags = Feed(tags_vec, "tags");
    
    std::cout << "Array Feed:\n";
    std::cout << "  tags: " << tags.to_string() << "\n\n";
    
    // MongoDB-style operations
    std::cout << "MongoDB-inspired operations:\n\n";
    
    // $inc: Increment numeric field
    auto new_age = age.inc(1);
    std::cout << "  age.inc(1): " << new_age.to_string() << "\n";
    
    // $mul: Multiply numeric field
    auto discounted_price = price.mul(0.9);  // 10% discount
    std::cout << "  price.mul(0.9): " << discounted_price.to_string() << " (10% off)\n";
    
    // $push: Add to array
    auto new_tags = tags.push(Feed::ScalarValue(std::string("new")));
    std::cout << "  tags.push('new'): " << new_tags.to_string() << "\n";
    
    // String operations
    auto upper_name = name.to_upper();
    std::cout << "  name.to_upper(): " << upper_name.to_string() << "\n\n";
    
    // ============================================================
    // Example 2: Nested Documents (Feed within Feed)
    // ============================================================
    print_separator("2. Nested Documents");
    
    Feed::DocumentValue address_doc;
    address_doc["city"] = std::make_shared<Feed>("Seattle", "city");
    address_doc["state"] = std::make_shared<Feed>("WA", "state");
    address_doc["zip"] = std::make_shared<Feed>(98101, "zip");
    
    auto address = Feed(address_doc, "address");
    
    std::cout << "Nested document (address):\n";
    std::cout << "  Full: " << address.to_string() << "\n";
    std::cout << "  City: " << address.get_field("city").to_string() << "\n";
    std::cout << "  Has state: " << (address.has_field("state") ? "yes" : "no") << "\n\n";
    
    // Modify nested document
    auto updated_address = address.set_field("country", Feed("USA", "country"));
    std::cout << "After adding country:\n";
    std::cout << "  " << updated_address.to_string() << "\n\n";
    
    // ============================================================
    // Example 3: Catalog (2D) - Document Collection
    // ============================================================
    print_separator("3. Catalog (2D) - Document Collection");
    
    auto products = std::make_shared<Catalog>("products");
    
    std::cout << "Creating product catalog:\n\n";
    
    // Insert documents
    Catalog::Document laptop = {
        {"name", Feed("Laptop Pro", "name")},
        {"price", Feed(1299.99, "price")},
        {"stock", Feed(50, "stock")},
        {"category", Feed("electronics", "category")},
        {"rating", Feed(4.5, "rating")}
    };
    
    Catalog::Document phone = {
        {"name", Feed("Smartphone X", "name")},
        {"price", Feed(899.99, "price")},
        {"stock", Feed(120, "stock")},
        {"category", Feed("electronics", "category")},
        {"rating", Feed(4.7, "rating")}
    };
    
    Catalog::Document tablet = {
        {"name", Feed("Tablet Plus", "name")},
        {"price", Feed(499.99, "price")},
        {"stock", Feed(75, "stock")},
        {"category", Feed("electronics", "category")},
        {"rating", Feed(4.3, "rating")}
    };
    
    products = products->insert_one(laptop);
    products = products->insert_many({phone, tablet});
    
    std::cout << "Inserted 3 products\n";
    std::cout << "Collection size: " << products->size() << "\n\n";
    
    // ============================================================
    // Example 4: Query Operations (MongoDB-style)
    // ============================================================
    print_separator("4. Query Operations");
    
    std::cout << "Finding products:\n\n";
    
    // Simple query: price >= 500
    auto expensive_query = Catalog::gte("price", Feed::ScalarValue(500.0));
    auto expensive_products = products->find(expensive_query);
    std::cout << "Products with price >= 500: " << expensive_products.size() << " found\n\n";
    
    // Complex query: price between 500 and 1000 AND rating > 4.5
    auto complex_query = Catalog::and_({
        Catalog::gte("price", Feed::ScalarValue(500.0)),
        Catalog::lte("price", Feed::ScalarValue(1000.0)),
        Catalog::gt("rating", Feed::ScalarValue(4.5))
    });
    auto filtered = products->find(complex_query);
    std::cout << "Products $500-$1000 with rating > 4.5: " << filtered.size() << " found\n\n";
    
    // Count documents
    auto count = products->count(Catalog::eq("category", Feed::ScalarValue(std::string("electronics"))));
    std::cout << "Electronics count: " << count << "\n\n";
    
    // ============================================================
    // Example 5: Update Operations
    // ============================================================
    print_separator("5. Update Operations");
    
    std::cout << "Updating products:\n\n";
    
    // Update one: Set sale flag
    auto update_query = Catalog::eq("name", Feed::ScalarValue(std::string("Laptop Pro")));
    auto set_sale = Catalog::set("on_sale", Feed(true, "on_sale"));
    products = products->update_one(update_query, set_sale);
    std::cout << "Set Laptop Pro on sale\n";
    
    // Update many: Increase stock by 10
    auto all_products = [](const Catalog::Document&) { return true; };
    auto inc_stock = Catalog::inc("stock", 10.0);
    products = products->update_many(all_products, inc_stock);
    std::cout << "Increased all product stock by 10\n";
    
    // Update with $mul: Apply 15% discount to expensive products
    auto discount_query = Catalog::gte("price", Feed::ScalarValue(1000.0));
    auto apply_discount = Catalog::mul("price", 0.85);
    products = products->update_many(discount_query, apply_discount);
    std::cout << "Applied 15% discount to products >= $1000\n\n";
    
    // ============================================================
    // Example 6: Aggregation Operations (Feed-First)
    // ============================================================
    print_separator("6. Aggregation Operations");
    
    std::cout << "Statistical aggregations (delegates to Feed operations):\n\n";
    
    // Aggregate across all documents
    double avg_price = products->avg("price");
    double total_stock = products->sum("stock");
    double min_price = products->min("price");
    double max_price = products->max("price");
    
    std::cout << "  Average price: $" << avg_price << "\n";
    std::cout << "  Total stock: " << total_stock << " units\n";
    std::cout << "  Price range: $" << min_price << " - $" << max_price << "\n\n";
    
    // Group by category
    auto grouped = products->group("category");
    std::cout << "Grouped by category: " << grouped->size() << " groups\n\n";
    
    // Aggregation pipeline
    std::cout << "Aggregation pipeline:\n";
    auto pipeline_result = products->aggregate()
        ->match(Catalog::gte("rating", Feed::ScalarValue(4.0)))
        ->project({"name", "price", "rating"})
        ->sort("rating", false)  // descending
        ->limit(2)
        ->execute();
    std::cout << "  Top 2 products by rating (>= 4.0): " << pipeline_result->size() << " results\n\n";
    
    // ============================================================
    // Example 7: Archive (3D) - Database of Collections
    // ============================================================
    print_separator("7. Archive (3D) - Database of Collections");
    
    auto ecommerce_db = std::make_shared<Archive>("ecommerce");
    
    std::cout << "Creating database with multiple collections:\n\n";
    
    // Create collections
    ecommerce_db = ecommerce_db->create_collection("products");
    ecommerce_db = ecommerce_db->create_collection("customers");
    ecommerce_db = ecommerce_db->create_collection("orders");
    
    // Set collections
    ecommerce_db = ecommerce_db->set_collection("products", products);
    
    auto collections = ecommerce_db->list_collections();
    std::cout << "Collections: ";
    for (const auto& name : collections) {
        std::cout << name << " ";
    }
    std::cout << "\n\n";
    
    // Database statistics
    std::cout << "Database statistics:\n";
    std::cout << "  Total collections: " << ecommerce_db->num_collections() << "\n";
    std::cout << "  Total documents: " << ecommerce_db->total_documents() << "\n";
    
    auto sizes = ecommerce_db->collection_sizes();
    std::cout << "  Collection sizes:\n";
    for (const auto& [name, size] : sizes) {
        std::cout << "    " << name << ": " << size << " documents\n";
    }
    std::cout << "\n";
    
    // ============================================================
    // Example 8: Cross-Collection Operations
    // ============================================================
    print_separator("8. Cross-Collection Operations");
    
    std::cout << "Database-level operations:\n\n";
    
    // Search across all collections
    auto global_query = Catalog::exists("price", true);
    auto global_results = ecommerce_db->find_in_all(global_query);
    std::cout << "Documents with 'price' field across all collections:\n";
    for (const auto& [coll_name, docs] : global_results) {
        std::cout << "  " << coll_name << ": " << docs.size() << " documents\n";
    }
    std::cout << "\n";
    
    // Aggregate across database
    double total_db_revenue = ecommerce_db->sum_across("price");
    std::cout << "Total revenue potential (sum of all prices): $" << total_db_revenue << "\n\n";
    
    // Apply operation to all collections
    auto updated_db = ecommerce_db->map_collections([](const auto& catalog) {
        // Add timestamp to all documents
        return catalog->update_many(
            [](const auto&) { return true; },
            Catalog::current_date("updated_at")
        );
    });
    std::cout << "Added 'updated_at' timestamp to all documents in all collections\n\n";
    
    // ============================================================
    // Example 9: Comparison with Thread-First Design
    // ============================================================
    print_separator("9. Feed-First vs Thread-First");
    
    std::cout << "Design Philosophy Comparison:\n\n";
    
    std::cout << "Thread-First (Table - SQL-like):\n";
    std::cout << "  Thread (1D)    → Column in DataFrame\n";
    std::cout << "  DataFrame (2D) → Table with shared index (columnar)\n";
    std::cout << "  Atlas (3D)     → Collection of DataFrames\n";
    std::cout << "  Operations: SQL-inspired, vectorized column operations\n";
    std::cout << "  Example: df->get_col(\"price\").mean()\n\n";
    
    std::cout << "Feed-First (Handle - MongoDB-like):\n";
    std::cout << "  Feed (1D)      → Field in Document\n";
    std::cout << "  Catalog (2D)   → Collection of Documents (document-oriented)\n";
    std::cout << "  Archive (3D)   → Database of Collections\n";
    std::cout << "  Operations: MongoDB-inspired, flexible document operations\n";
    std::cout << "  Example: doc[\"price\"].inc(10.0)\n\n";
    
    // ============================================================
    // Example 10: Transactions and Backup
    // ============================================================
    print_separator("10. Transactions and Backup");
    
    std::cout << "Database management:\n\n";
    
    // Transaction
    std::cout << "Starting transaction:\n";
    auto txn = ecommerce_db->begin_transaction();
    txn->insert("orders", Catalog::Document{
        {"order_id", Feed(1001, "order_id")},
        {"customer_id", Feed(5, "customer_id")},
        {"total", Feed(1299.99, "total")}
    });
    txn->update("products", 
        Catalog::eq("name", Feed::ScalarValue(std::string("Laptop Pro"))),
        Catalog::inc("stock", -1.0));
    std::cout << "  - Insert order\n";
    std::cout << "  - Update product stock\n";
    auto updated = txn->commit();
    std::cout << "  Transaction committed\n\n";
    
    // Backup
    std::cout << "Creating backup:\n";
    std::string backup_json = ecommerce_db->export_to_json();
    std::cout << "  Exported to JSON (" << backup_json.length() << " bytes)\n";
    
    // Snapshot
    auto snapshot = ecommerce_db->snapshot();
    std::cout << "  Created snapshot\n\n";
    
    print_separator("Summary");
    
    std::cout << "Feed-First Design provides:\n\n";
    std::cout << "  ✅ Flexible, schema-free document storage\n";
    std::cout << "  ✅ MongoDB-inspired query and update operations\n";
    std::cout << "  ✅ Nested documents and array operations\n";
    std::cout << "  ✅ Aggregation pipelines\n";
    std::cout << "  ✅ Cross-collection queries\n";
    std::cout << "  ✅ Transaction support\n";
    std::cout << "  ✅ Backup/restore capabilities\n\n";
    
    std::cout << "The Feed-First design complements Thread-First:\n";
    std::cout << "  - Use Table (Thread-First) for columnar/relational data (SQL-like)\n";
    std::cout << "  - Use Handle (Feed-First) for document/hierarchical data (MongoDB-like)\n\n";
    
    std::cout << "Both follow the same 1D → 2D → 3D architecture!\n\n";
    
    return 0;
}
