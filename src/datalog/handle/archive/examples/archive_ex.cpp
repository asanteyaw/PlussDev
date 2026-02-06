/**
 * @file archive_ex.cpp
 * @brief Archive (3D) - Database of Collections Examples
 * 
 * Demonstrates Archive operations (MongoDB database-like).
 * Archive is analogous to Atlas in the Table module.
 */

#include <iostream>
#include <pluss/pluss.h>
#include <pluss/datalog/handle/archive/archive.h>

using namespace pluss;
using namespace pluss::handle;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(70, '=') << "\n\n";
}

int main() {
    print_separator("Archive (3D) - Database of Collections");
    
    std::cout << "Archive represents a MongoDB-like database:\n";
    std::cout << "  Feed (1D)      → Document field\n";
    std::cout << "  Catalog (2D)   → Collection of documents\n";
    std::cout << "  Archive (3D)   → Database of collections (Feed-First)\n\n";
    
    // ============================================================
    // Example 1: Creating Archive and Collections
    // ============================================================
    print_separator("1. Creating Archive and Managing Collections");
    
    auto ecommerce = std::make_shared<Archive>("ecommerce_db");
    std::cout << "Created database: " << ecommerce->name() << "\n";
    std::cout << "Initial collections: " << ecommerce->num_collections() << "\n\n";
    
    // Create collections
    ecommerce = ecommerce->create_collection("products");
    ecommerce = ecommerce->create_collection("customers");
    ecommerce = ecommerce->create_collection("orders");
    ecommerce = ecommerce->create_collection("reviews");
    
    std::cout << "Created collections:\n";
    auto collections = ecommerce->list_collections();
    for (const auto& name : collections) {
        std::cout << "  - " << name << "\n";
    }
    std::cout << "\nTotal collections: " << ecommerce->num_collections() << "\n\n";
    
    // ============================================================
    // Example 2: Populating Collections
    // ============================================================
    print_separator("2. Populating Collections with Data");
    
    // Get products collection
    auto products = ecommerce->get_collection("products");
    
    std::cout << "Adding products to 'products' collection:\n";
    
    // Create product documents
    std::vector<Catalog::Document> product_docs = {
        {
            {"_id", Feed(1, "_id")},
            {"name", Feed("Laptop Pro", "name")},
            {"price", Feed(1299.99, "price")},
            {"category", Feed("electronics", "category")},
            {"stock", Feed(50, "stock")}
        },
        {
            {"_id", Feed(2, "_id")},
            {"name", Feed("Wireless Mouse", "name")},
            {"price", Feed(29.99, "price")},
            {"category", Feed("accessories", "category")},
            {"stock", Feed(200, "stock")}
        },
        {
            {"_id", Feed(3, "_id")},
            {"name", Feed("USB-C Hub", "name")},
            {"price", Feed(49.99, "price")},
            {"category", Feed("accessories", "category")},
            {"stock", Feed(150, "stock")}
        }
    };
    
    products = products->insert_many(product_docs);
    ecommerce = ecommerce->set_collection("products", products);
    std::cout << "  Inserted " << product_docs.size() << " products\n\n";
    
    // Add customers
    auto customers = ecommerce->get_collection("customers");
    
    std::cout << "Adding customers to 'customers' collection:\n";
    
    std::vector<Catalog::Document> customer_docs = {
        {
            {"_id", Feed(101, "_id")},
            {"name", Feed("Alice Johnson", "name")},
            {"email", Feed("alice@example.com", "email")},
            {"country", Feed("USA", "country")}
        },
        {
            {"_id", Feed(102, "_id")},
            {"name", Feed("Bob Smith", "name")},
            {"email", Feed("bob@example.com", "email")},
            {"country", Feed("Canada", "country")}
        }
    };
    
    customers = customers->insert_many(customer_docs);
    ecommerce = ecommerce->set_collection("customers", customers);
    std::cout << "  Inserted " << customer_docs.size() << " customers\n\n";
    
    // Add orders
    auto orders = ecommerce->get_collection("orders");
    
    std::cout << "Adding orders to 'orders' collection:\n";
    
    std::vector<Catalog::Document> order_docs = {
        {
            {"_id", Feed(1001, "_id")},
            {"customer_id", Feed(101, "customer_id")},
            {"product_id", Feed(1, "product_id")},
            {"quantity", Feed(1, "quantity")},
            {"total", Feed(1299.99, "total")}
        },
        {
            {"_id", Feed(1002, "_id")},
            {"customer_id", Feed(102, "customer_id")},
            {"product_id", Feed(2, "product_id")},
            {"quantity", Feed(2, "quantity")},
            {"total", Feed(59.98, "total")}
        },
        {
            {"_id", Feed(1003, "_id")},
            {"customer_id", Feed(101, "customer_id")},
            {"product_id", Feed(3, "product_id")},
            {"quantity", Feed(1, "quantity")},
            {"total", Feed(49.99, "total")}
        }
    };
    
    orders = orders->insert_many(order_docs);
    ecommerce = ecommerce->set_collection("orders", orders);
    std::cout << "  Inserted " << order_docs.size() << " orders\n\n";
    
    // ============================================================
    // Example 3: Database Statistics
    // ============================================================
    print_separator("3. Database Statistics");
    
    std::cout << "Database overview:\n";
    std::cout << "  Name: " << ecommerce->name() << "\n";
    std::cout << "  Collections: " << ecommerce->num_collections() << "\n";
    std::cout << "  Total documents: " << ecommerce->total_documents() << "\n";
    std::cout << "  Database size: " << ecommerce->total_size_mb() << " MB\n\n";
    
    std::cout << "Collection sizes:\n";
    auto sizes = ecommerce->collection_sizes();
    for (const auto& [name, size] : sizes) {
        std::cout << "  " << name << ": " << size << " documents\n";
    }
    std::cout << "\n";
    
    std::cout << "Specific collection document count:\n";
    std::cout << "  products: " << ecommerce->count_documents("products") << "\n";
    std::cout << "  customers: " << ecommerce->count_documents("customers") << "\n";
    std::cout << "  orders: " << ecommerce->count_documents("orders") << "\n\n";
    
    // ============================================================
    // Example 4: Cross-Collection Queries
    // ============================================================
    print_separator("4. Cross-Collection Queries");
    
    std::cout << "Search for documents with 'price' field across all collections:\n";
    auto price_query = Catalog::exists("price", true);
    auto results = ecommerce->find_in_all(price_query);
    
    std::cout << "  Results:\n";
    for (const auto& [coll_name, docs] : results) {
        std::cout << "    " << coll_name << ": " << docs.size() << " documents\n";
    }
    std::cout << "\n";
    
    std::cout << "Count documents with 'email' field in all collections:\n";
    auto email_query = Catalog::exists("email", true);
    auto counts = ecommerce->count_in_all(email_query);
    std::cout << "  Results:\n";
    for (const auto& [coll_name, count] : counts) {
        std::cout << "    " << coll_name << ": " << count << " documents\n";
    }
    std::cout << "\n";
    
    // ============================================================
    // Example 5: Database-Level Aggregations
    // ============================================================
    print_separator("5. Database-Level Aggregations");
    
    std::cout << "Aggregate 'total' field across all collections:\n";
    double total_revenue = ecommerce->sum_across("total");
    std::cout << "  Total revenue: $" << total_revenue << "\n\n";
    
    std::cout << "Average 'price' across all collections:\n";
    double avg_price = ecommerce->avg_across("price");
    std::cout << "  Average price: $" << avg_price << "\n\n";
    
    std::cout << "Total document count across database:\n";
    int64_t total_docs = ecommerce->count_across();
    std::cout << "  Total: " << total_docs << " documents\n\n";
    
    // ============================================================
    // Example 6: Union Collections
    // ============================================================
    print_separator("6. Union Collections");
    
    std::cout << "Combining products and accessories:\n";
    // Note: This would work if we had separate collections
    std::cout << "  Union creates a merged view of multiple collections\n";
    std::cout << "  Example: union_collections({'products', 'accessories'})\n\n";
    
    // ============================================================
    // Example 7: Map Collections (Apply Operation to All)
    // ============================================================
    print_separator("7. Map Collections");
    
    std::cout << "Adding 'last_updated' timestamp to all documents in all collections:\n";
    
    auto updated_db = ecommerce->map_collections([](const auto& catalog) {
        // Add timestamp to all documents
        return catalog->update_many(
            [](const auto&) { return true; },  // all documents
            Catalog::current_date("last_updated")
        );
    });
    
    std::cout << "  Applied timestamp to all collections\n\n";
    
    // ============================================================
    // Example 8: Filter Collections
    // ============================================================
    print_separator("8. Filter Collections");
    
    std::cout << "Filter collections with more than 2 documents:\n";
    
    auto large_collections = ecommerce->filter_collections([](const auto& catalog) {
        return catalog->size() > 2;
    });
    
    std::cout << "  Filtered collections: " << large_collections->num_collections() << "\n";
    auto filtered_names = large_collections->list_collections();
    for (const auto& name : filtered_names) {
        std::cout << "    - " << name << "\n";
    }
    std::cout << "\n";
    
    // ============================================================
    // Example 9: Indexing Across Collections
    // ============================================================
    print_separator("9. Database-Wide Indexing");
    
    std::cout << "Creating indexes on '_id' field in all collections:\n";
    ecommerce->create_index_all("_id", true);  // unique index
    std::cout << "  Created unique indexes on '_id'\n\n";
    
    std::cout << "Creating indexes on 'name' field in all collections:\n";
    ecommerce->create_index_all("name", false);
    std::cout << "  Created indexes on 'name'\n\n";
    
    std::cout << "Dropping indexes on 'name' from all collections:\n";
    ecommerce->drop_index_all("name");
    std::cout << "  Dropped indexes\n\n";
    
    // ============================================================
    // Example 10: Transactions
    // ============================================================
    print_separator("10. Transactions");
    
    std::cout << "Starting transaction:\n";
    auto txn = ecommerce->begin_transaction();
    
    // Create a new order
    Catalog::Document new_order = {
        {"_id", Feed(1004, "_id")},
        {"customer_id", Feed(102, "customer_id")},
        {"product_id", Feed(1, "product_id")},
        {"quantity", Feed(1, "quantity")},
        {"total", Feed(1299.99, "total")}
    };
    
    txn->insert("orders", new_order);
    std::cout << "  - Insert new order\n";
    
    // Update product stock
    txn->update("products",
        Catalog::eq("_id", Feed::ScalarValue(1)),
        Catalog::inc("stock", -1.0)
    );
    std::cout << "  - Decrease product stock\n";
    
    // Update customer
    txn->update("customers",
        Catalog::eq("_id", Feed::ScalarValue(102)),
        Catalog::inc("total_orders", 1.0)
    );
    std::cout << "  - Update customer order count\n\n";
    
    std::cout << "Committing transaction:\n";
    auto updated = txn->commit();
    std::cout << "  Transaction committed successfully\n\n";
    
    // ============================================================
    // Example 11: Backup and Restore
    // ============================================================
    print_separator("11. Backup and Restore");
    
    std::cout << "Creating snapshot:\n";
    auto snapshot = ecommerce->snapshot();
    std::cout << "  Snapshot created\n\n";
    
    std::cout << "Exporting database to JSON:\n";
    std::string json_backup = ecommerce->export_to_json();
    std::cout << "  JSON size: " << json_backup.length() << " bytes\n\n";
    
    std::cout << "Exporting database to BSON:\n";
    std::string bson_backup = ecommerce->export_to_bson();
    std::cout << "  BSON size: " << bson_backup.length() << " bytes\n\n";
    
    std::cout << "Saving to file:\n";
    ecommerce->save_to_file("ecommerce_backup.json", "json");
    std::cout << "  Saved to ecommerce_backup.json\n\n";
    
    std::cout << "Restoring from snapshot:\n";
    auto restored = ecommerce->restore(snapshot);
    std::cout << "  Database restored\n\n";
    
    // ============================================================
    // Example 12: Collection Management
    // ============================================================
    print_separator("12. Collection Management");
    
    std::cout << "Renaming collection:\n";
    ecommerce = ecommerce->rename_collection("reviews", "product_reviews");
    std::cout << "  Renamed 'reviews' to 'product_reviews'\n\n";
    
    std::cout << "Dropping collection:\n";
    ecommerce = ecommerce->drop_collection("product_reviews");
    std::cout << "  Dropped 'product_reviews' collection\n";
    std::cout << "  Remaining collections: " << ecommerce->num_collections() << "\n\n";
    
    std::cout << "Checking collection existence:\n";
    std::cout << "  has_collection('products'): " 
              << (ecommerce->has_collection("products") ? "yes" : "no") << "\n";
    std::cout << "  has_collection('reviews'): " 
              << (ecommerce->has_collection("reviews") ? "yes" : "no") << "\n\n";
    
    // ============================================================
    // Example 13: Database Metadata
    // ============================================================
    print_separator("13. Database Metadata");
    
    std::cout << "Database stats:\n";
    auto stats = ecommerce->stats();
    for (const auto& [key, value] : stats) {
        std::cout << "  " << key << ": " << value << "\n";
    }
    std::cout << "\n";
    
    std::cout << "Database schema:\n";
    std::cout << ecommerce->schema() << "\n\n";
    
    // ============================================================
    // Example 14: Database Display
    // ============================================================
    print_separator("14. Database Display");
    
    std::cout << "Database information:\n";
    ecommerce->info();
    std::cout << "\n";
    
    std::cout << "Database structure:\n";
    ecommerce->display();
    std::cout << "\n";
    
    // ============================================================
    // Example 15: Merge Archives
    // ============================================================
    print_separator("15. Merge Archives");
    
    std::cout << "Creating second database:\n";
    auto analytics = std::make_shared<Archive>("analytics_db");
    analytics = analytics->create_collection("logs");
    analytics = analytics->create_collection("metrics");
    std::cout << "  Created analytics_db with 2 collections\n\n";
    
    std::cout << "Merging databases:\n";
    auto merged = ecommerce->merge_archives(analytics);
    std::cout << "  Merged database collections: " << merged->num_collections() << "\n";
    auto merged_colls = merged->list_collections();
    for (const auto& name : merged_colls) {
        std::cout << "    - " << name << "\n";
    }
    std::cout << "\n";
    
    // ============================================================
    // Example 16: Database Maintenance
    // ============================================================
    print_separator("16. Database Maintenance");
    
    std::cout << "Validating database integrity:\n";
    ecommerce->validate();
    std::cout << "  Database validated\n\n";
    
    std::cout << "Compacting database:\n";
    ecommerce->compact();
    std::cout << "  Database compacted (optimized storage)\n\n";
    
    print_separator("Summary");
    
    std::cout << "Archive (3D) provides MongoDB-like database operations:\n\n";
    std::cout << "  ✅ Collection management: create, drop, rename\n";
    std::cout << "  ✅ Cross-collection queries: find_in_all, count_in_all\n";
    std::cout << "  ✅ Database aggregations: sum_across, avg_across\n";
    std::cout << "  ✅ Collection operations: union, intersect, merge\n";
    std::cout << "  ✅ Map/filter operations: apply to all collections\n";
    std::cout << "  ✅ Database-wide indexing\n";
    std::cout << "  ✅ Transactions: commit/rollback\n";
    std::cout << "  ✅ Backup/restore: JSON, BSON, snapshots\n";
    std::cout << "  ✅ Database statistics and metadata\n";
    std::cout << "  ✅ Maintenance: validate, compact\n\n";
    
    std::cout << "Feed-First Design Complete!\n";
    std::cout << "  Feed (1D)    → feed_ex.cpp\n";
    std::cout << "  Catalog (2D) → catalog_ex.cpp\n";
    std::cout << "  Archive (3D) → archive_ex.cpp\n\n";
    
    std::cout << "This complements the Thread-First design (Table module):\n";
    std::cout << "  - Use Table (Thread-First) for SQL-like operations\n";
    std::cout << "  - Use Handle (Feed-First) for MongoDB-like operations\n\n";
    
    return 0;
}
