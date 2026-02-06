/**
 * @file catalog_ex.cpp
 * @brief Catalog (2D) - Document Collection Examples
 * 
 * Demonstrates Catalog operations (MongoDB collection-like).
 * Catalog is analogous to DataFrame in the Table module.
 */

#include <iostream>
#include <pluss/pluss.h>
#include <pluss/datalog/handle/catalog/catalog.h>

using namespace pluss;
using namespace pluss::handle;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(70, '=') << "\n\n";
}

int main() {
    print_separator("Catalog (2D) - Document Collection");
    
    std::cout << "Catalog represents a MongoDB-like collection of documents:\n";
    std::cout << "  Feed (1D)      → Document field\n";
    std::cout << "  Catalog (2D)   → Collection of documents (Feed-First)\n";
    std::cout << "  Archive (3D)   → Database of collections\n\n";
    
    // ============================================================
    // Example 1: Creating a Catalog and Inserting Documents
    // ============================================================
    print_separator("1. Creating Catalog and Inserting Documents");
    
    auto products = std::make_shared<Catalog>("products");
    std::cout << "Created empty catalog: " << products->name() << "\n";
    std::cout << "Initial size: " << products->size() << "\n\n";
    
    // Create documents
    Catalog::Document laptop = {
        {"_id", Feed(1, "_id")},
        {"name", Feed("Laptop Pro 15", "name")},
        {"price", Feed(1299.99, "price")},
        {"category", Feed("electronics", "category")},
        {"stock", Feed(45, "stock")},
        {"rating", Feed(4.5, "rating")},
        {"tags", Feed(std::vector<std::string>{"computers", "portable", "work"}, "tags")}
    };
    
    Catalog::Document phone = {
        {"_id", Feed(2, "_id")},
        {"name", Feed("Smartphone X", "name")},
        {"price", Feed(899.99, "price")},
        {"category", Feed("electronics", "category")},
        {"stock", Feed(120, "stock")},
        {"rating", Feed(4.7, "rating")},
        {"tags", Feed(std::vector<std::string>{"mobile", "communication"}, "tags")}
    };
    
    Catalog::Document tablet = {
        {"_id", Feed(3, "_id")},
        {"name", Feed("Tablet Plus", "name")},
        {"price", Feed(499.99, "price")},
        {"category", Feed("electronics", "category")},
        {"stock", Feed(75, "stock")},
        {"rating", Feed(4.3, "rating")},
        {"tags", Feed(std::vector<std::string>{"portable", "reading", "entertainment"}, "tags")}
    };
    
    Catalog::Document monitor = {
        {"_id", Feed(4, "_id")},
        {"name", Feed("4K Monitor 27\"", "name")},
        {"price", Feed(349.99, "price")},
        {"category", Feed("electronics", "category")},
        {"stock", Feed(30, "stock")},
        {"rating", Feed(4.6, "rating")},
        {"tags", Feed(std::vector<std::string>{"display", "work"}, "tags")}
    };
    
    // Insert one
    products = products->insert_one(laptop);
    std::cout << "Inserted 1 document (laptop)\n";
    std::cout << "Size: " << products->size() << "\n\n";
    
    // Insert many
    products = products->insert_many({phone, tablet, monitor});
    std::cout << "Inserted 3 more documents\n";
    std::cout << "Total size: " << products->size() << " documents\n\n";
    
    // ============================================================
    // Example 2: Finding Documents (Simple Queries)
    // ============================================================
    print_separator("2. Finding Documents - Simple Queries");
    
    std::cout << "Query 1: Find all electronics:\n";
    auto all_electronics = products->find(
        Catalog::eq("category", Feed::ScalarValue(std::string("electronics")))
    );
    std::cout << "  Found: " << all_electronics.size() << " products\n\n";
    
    std::cout << "Query 2: Find products priced >= $500:\n";
    auto expensive = products->find(
        Catalog::gte("price", Feed::ScalarValue(500.0))
    );
    std::cout << "  Found: " << expensive.size() << " expensive products\n";
    for (const auto& doc : expensive) {
        std::cout << "    - " << doc.at("name").to_string() 
                  << ": $" << doc.at("price").to_string() << "\n";
    }
    std::cout << "\n";
    
    std::cout << "Query 3: Find top-rated products (rating > 4.5):\n";
    auto top_rated = products->find(
        Catalog::gt("rating", Feed::ScalarValue(4.5))
    );
    std::cout << "  Found: " << top_rated.size() << " top-rated products\n";
    for (const auto& doc : top_rated) {
        std::cout << "    - " << doc.at("name").to_string() 
                  << ": " << doc.at("rating").to_string() << " stars\n";
    }
    std::cout << "\n";
    
    // ============================================================
    // Example 3: Complex Queries (Logical Operators)
    // ============================================================
    print_separator("3. Complex Queries - Logical Operators");
    
    std::cout << "Query: Price between $400-$1000 AND rating > 4.4:\n";
    auto filtered_products = products->find(
        Catalog::and_({
            Catalog::gte("price", Feed::ScalarValue(400.0)),
            Catalog::lte("price", Feed::ScalarValue(1000.0)),
            Catalog::gt("rating", Feed::ScalarValue(4.4))
        })
    );
    std::cout << "  Found: " << filtered_products.size() << " products\n";
    for (const auto& doc : filtered_products) {
        std::cout << "    - " << doc.at("name").to_string() 
                  << ": $" << doc.at("price").to_string()
                  << ", " << doc.at("rating").to_string() << " stars\n";
    }
    std::cout << "\n";
    
    std::cout << "Query: Rating >= 4.6 OR price < $400:\n";
    auto or_query = products->find(
        Catalog::or_({
            Catalog::gte("rating", Feed::ScalarValue(4.6)),
            Catalog::lt("price", Feed::ScalarValue(400.0))
        })
    );
    std::cout << "  Found: " << or_query.size() << " products\n\n";
    
    std::cout << "Query: Products $in price list:\n";
    auto in_prices = products->find(
        Catalog::in("price", {
            Feed::ScalarValue(499.99),
            Feed::ScalarValue(899.99)
        })
    );
    std::cout << "  Found: " << in_prices.size() << " products at specific prices\n\n";
    
    // ============================================================
    // Example 4: Find with Projection
    // ============================================================
    print_separator("4. Find with Projection (Select Fields)");
    
    std::cout << "Query: All products, but only show name and price:\n";
    auto all_query = [](const Catalog::Document&) { return true; };
    Catalog::Projection fields = {"name", "price"};
    auto projected = products->find(all_query, fields);
    
    std::cout << "  Results (name, price only):\n";
    for (const auto& doc : projected) {
        std::cout << "    - " << doc.at("name").to_string() 
                  << ": $" << doc.at("price").to_string() << "\n";
    }
    std::cout << "\n";
    
    // ============================================================
    // Example 5: Find One and Count
    // ============================================================
    print_separator("5. Find One and Count Operations");
    
    std::cout << "Find one product with price > $1000:\n";
    auto high_end = products->find_one(
        Catalog::gt("price", Feed::ScalarValue(1000.0))
    );
    if (!high_end.empty()) {
        std::cout << "  Found: " << high_end.at("name").to_string() << "\n\n";
    } else {
        std::cout << "  No products found above $1000\n\n";
    }
    
    std::cout << "Count products in stock (stock > 0):\n";
    auto in_stock_count = products->count(
        Catalog::gt("stock", Feed::ScalarValue(0))
    );
    std::cout << "  Count: " << in_stock_count << " products in stock\n\n";
    
    std::cout << "Check if any product costs exactly $899.99:\n";
    bool exists = products->exists(
        Catalog::eq("price", Feed::ScalarValue(899.99))
    );
    std::cout << "  Exists: " << (exists ? "yes" : "no") << "\n\n";
    
    // ============================================================
    // Example 6: Update Operations
    // ============================================================
    print_separator("6. Update Operations");
    
    std::cout << "Update 1: Set 'on_sale' flag for laptop:\n";
    products = products->update_one(
        Catalog::eq("name", Feed::ScalarValue(std::string("Laptop Pro 15"))),
        Catalog::set("on_sale", Feed(true, "on_sale"))
    );
    std::cout << "  Updated laptop with on_sale flag\n\n";
    
    std::cout << "Update 2: Increase stock by 10 for all products:\n";
    products = products->update_many(
        all_query,
        Catalog::inc("stock", 10.0)
    );
    std::cout << "  Increased stock for all products\n\n";
    
    std::cout << "Update 3: Apply 15% discount to expensive items (price >= $1000):\n";
    products = products->update_many(
        Catalog::gte("price", Feed::ScalarValue(1000.0)),
        Catalog::mul("price", 0.85)
    );
    std::cout << "  Applied 15% discount\n\n";
    
    std::cout << "Update 4: Rename field 'rating' to 'stars':\n";
    products = products->update_many(
        all_query,
        Catalog::rename("rating", "stars")
    );
    std::cout << "  Renamed field\n\n";
    
    // ============================================================
    // Example 7: Array Update Operations
    // ============================================================
    print_separator("7. Array Update Operations");
    
    std::cout << "Update: Add 'featured' tag to top-rated products:\n";
    products = products->update_many(
        Catalog::gte("stars", Feed::ScalarValue(4.6)),
        Catalog::push("tags", Feed::ScalarValue(std::string("featured")))
    );
    std::cout << "  Added 'featured' tag\n\n";
    
    std::cout << "Update: Remove 'portable' tag from tablets:\n";
    products = products->update_one(
        Catalog::eq("name", Feed::ScalarValue(std::string("Tablet Plus"))),
        Catalog::pull("tags", Feed::ScalarValue(std::string("portable")))
    );
    std::cout << "  Removed 'portable' tag\n\n";
    
    // ============================================================
    // Example 8: Delete Operations
    // ============================================================
    print_separator("8. Delete Operations");
    
    std::cout << "Initial catalog size: " << products->size() << "\n\n";
    
    std::cout << "Delete one: Remove product with lowest stock:\n";
    products = products->delete_one(
        Catalog::lt("stock", Feed::ScalarValue(50))
    );
    std::cout << "  Deleted 1 product\n";
    std::cout << "  New size: " << products->size() << "\n\n";
    
    std::cout << "Delete many: Remove all out-of-stock items (stock <= 0):\n";
    auto before_delete = products->size();
    products = products->delete_many(
        Catalog::lte("stock", Feed::ScalarValue(0))
    );
    auto deleted_count = before_delete - products->size();
    std::cout << "  Deleted " << deleted_count << " products\n";
    std::cout << "  Final size: " << products->size() << "\n\n";
    
    // ============================================================
    // Example 9: Aggregation Operations
    // ============================================================
    print_separator("9. Aggregation Operations");
    
    std::cout << "Statistical aggregations (Feed-First):\n\n";
    
    double avg_price = products->avg("price");
    double total_stock = products->sum("stock");
    double min_price = products->min("price");
    double max_price = products->max("price");
    int64_t product_count = products->count_field("name");
    
    std::cout << "  Average price:    $" << avg_price << "\n";
    std::cout << "  Total stock:      " << total_stock << " units\n";
    std::cout << "  Price range:      $" << min_price << " - $" << max_price << "\n";
    std::cout << "  Product count:    " << product_count << "\n\n";
    
    // ============================================================
    // Example 10: Grouping
    // ============================================================
    print_separator("10. Grouping");
    
    std::cout << "Group products by category:\n";
    auto grouped = products->group("category");
    std::cout << "  Groups created: " << grouped->size() << "\n\n";
    
    // ============================================================
    // Example 11: Sorting, Limiting, Skipping
    // ============================================================
    print_separator("11. Sorting, Limiting, and Skipping");
    
    std::cout << "Sort by price (ascending):\n";
    auto sorted_asc = products->sort("price", true);
    std::cout << "  Sorted catalog size: " << sorted_asc->size() << "\n\n";
    
    std::cout << "Sort by rating (descending):\n";
    auto sorted_desc = products->sort("stars", false);
    std::cout << "  Sorted catalog size: " << sorted_desc->size() << "\n\n";
    
    std::cout << "Get top 2 products:\n";
    auto top_2 = sorted_desc->limit(2);
    std::cout << "  Limited to: " << top_2->size() << " products\n\n";
    
    std::cout << "Skip first 1, then limit to 2:\n";
    auto paginated = sorted_desc->skip(1)->limit(2);
    std::cout << "  Page size: " << paginated->size() << " products\n\n";
    
    // ============================================================
    // Example 12: Aggregation Pipeline
    // ============================================================
    print_separator("12. Aggregation Pipeline");
    
    std::cout << "Pipeline: Filter → Project → Sort → Limit:\n\n";
    
    auto pipeline_result = products->aggregate()
        ->match(Catalog::gte("stars", Feed::ScalarValue(4.0)))
        ->project({"name", "price", "stars"})
        ->sort("stars", false)
        ->limit(3)
        ->execute();
    
    std::cout << "  Pipeline executed\n";
    std::cout << "  Results: " << pipeline_result->size() << " documents\n\n";
    
    // ============================================================
    // Example 13: Indexing
    // ============================================================
    print_separator("13. Indexing");
    
    std::cout << "Creating indexes:\n";
    products->create_index("price", false);
    products->create_index("category", false);
    products->create_index("name", true);  // unique
    std::cout << "  Created 3 indexes\n\n";
    
    auto indexes = products->list_indexes();
    std::cout << "Existing indexes:\n";
    for (const auto& idx : indexes) {
        std::cout << "  - " << idx << "\n";
    }
    std::cout << "\n";
    
    std::cout << "Dropping index on 'category':\n";
    products->drop_index("category");
    std::cout << "  Index dropped\n\n";
    
    // ============================================================
    // Example 14: Catalog I/O
    // ============================================================
    print_separator("14. Catalog I/O");
    
    std::cout << "Exporting catalog to JSON:\n";
    std::string json = products->to_json();
    std::cout << "  JSON size: " << json.length() << " bytes\n\n";
    
    std::cout << "Display catalog info:\n";
    products->info();
    std::cout << "\n";
    
    std::cout << "Display first 10 documents:\n";
    products->display(10);
    std::cout << "\n";
    
    print_separator("Summary");
    
    std::cout << "Catalog (2D) provides MongoDB-like collection operations:\n\n";
    std::cout << "  ✅ CRUD: insert, find, update, delete\n";
    std::cout << "  ✅ Query builders: eq, lt, gt, and, or, in\n";
    std::cout << "  ✅ Projections: select specific fields\n";
    std::cout << "  ✅ Update operators: $set, $inc, $mul, $push, $pull\n";
    std::cout << "  ✅ Aggregations: sum, avg, min, max, count\n";
    std::cout << "  ✅ Grouping and sorting\n";
    std::cout << "  ✅ Pagination: limit, skip\n";
    std::cout << "  ✅ Aggregation pipelines\n";
    std::cout << "  ✅ Indexing for performance\n";
    std::cout << "  ✅ JSON import/export\n\n";
    
    std::cout << "Catalog delegates to Feed operations (Feed-First design)!\n";
    std::cout << "Next: See archive_ex.cpp for Archive (3D) examples.\n\n";
    
    return 0;
}
