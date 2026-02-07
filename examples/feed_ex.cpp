/**
 * @file feed_first_example.cpp
 * @brief Comprehensive demonstration of Feed-First design philosophy
 * 
 * This example showcases the Feed → Catalog → Archive hierarchy.
 * Feed-First (Handle module) complements Thread-First (Table module).
 */

#include <iostream>
#include <pluss/pluss.h>

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
    std::cout << "  Feed (1D)    → Homogeneous field (holds Series, Note, or Timeline)\n";
    std::cout << "  Catalog (2D) → Dictionary of Feeds (like DataFrame)\n";
    std::cout << "  Archive (3D) → Dictionary of Catalogs\n\n";
    
    // ============================================================
    // Example 1: Feed (1D) - Homogeneous Fields
    // ============================================================
    print_separator("1. Feed (1D) - Homogeneous Fields");
    
    std::cout << "Creating Feeds (homogeneous fields):\n\n";
    
    // Numeric Feed (using Series)
    auto ages = Feed(pluss::value({25.0, 30.0, 35.0, 40.0}), "age");
    auto prices = Feed(pluss::value({99.99, 149.99, 199.99, 249.99}), "price");
    auto ratings = Feed(pluss::value({4.5, 4.7, 4.3, 4.8}), "rating");
    
    std::cout << "Numeric Feeds:\n";
    std::cout << "  ages: "; ages.display();
    std::cout << "  prices: "; prices.display();
    std::cout << "  ratings: "; ratings.display();
    std::cout << "\n";
    
    // Text Feed (using Note)
    auto names = Feed(pluss::make_note({"Alice", "Bob", "Charlie", "Diana"}), "name");
    auto categories = Feed(pluss::make_note({"electronics", "books", "clothing", "food"}), "category");
    
    std::cout << "Text Feeds:\n";
    std::cout << "  names: "; names.display();
    std::cout << "  categories: "; categories.display();
    std::cout << "\n";
    
    // Feed operations
    std::cout << "Feed operations:\n\n";
    
    // Arithmetic on numeric Feeds
    auto discounted = prices.as_series() * 0.9;  // 10% discount
    std::cout << "  Discounted prices (10% off): " << discounted << "\n";
    
    // Statistical operations
    std::cout << "  Average age: " << ages.mean() << "\n";
    std::cout << "  Total price: " << prices.sum() << "\n";
    std::cout << "  Min rating: " << ratings.min() << "\n";
    std::cout << "  Max rating: " << ratings.max() << "\n\n";
    
    // Filtering
    auto high_ratings_mask = ratings.as_series().gt(4.5);
    auto high_rated = ratings.filter(high_ratings_mask);
    std::cout << "  High ratings (>4.5): "; high_rated.display();
    std::cout << "\n";
    
    // ============================================================
    // Example 2: Catalog (2D) - Dictionary of Feeds
    // ============================================================
    print_separator("2. Catalog (2D) - Dictionary of Feeds");
    
    Catalog products("products");
    
    std::cout << "Creating product catalog:\n\n";
    
    // Add fields to catalog
    products["id"] = Feed(pluss::value({1.0, 2.0, 3.0, 4.0}), "id");
    products["name"] = Feed(pluss::make_note({"Laptop Pro", "Smartphone X", "Tablet Plus", "4K Monitor"}), "name");
    products["price"] = Feed(pluss::value({1299.99, 899.99, 499.99, 349.99}), "price");
    products["stock"] = Feed(pluss::value({50.0, 120.0, 75.0, 30.0}), "stock");
    products["category"] = Feed(pluss::make_note({"electronics", "electronics", "electronics", "electronics"}), "category");
    products["rating"] = Feed(pluss::value({4.5, 4.7, 4.3, 4.6}), "rating");
    
    std::cout << "Product catalog:\n";
    products.display();
    std::cout << "\n";
    
    // Catalog operations
    std::cout << "Catalog info:\n";
    std::cout << "  Fields: " << products.num_fields() << "\n";
    std::cout << "  Rows: " << products.size() << "\n";
    std::cout << "  Field names: ";
    for (const auto& key : products.keys()) {
        std::cout << key << " ";
    }
    std::cout << "\n\n";
    
    // Filtering catalog
    std::cout << "Filter products with price > 500:\n";
    auto price_series = products["price"].as_series();
    auto expensive_mask = price_series.gt(500.0);
    auto expensive = products.filter(expensive_mask);
    expensive.display();
    std::cout << "\n";
    
    // Sorting catalog
    std::cout << "Sort by price (descending):\n";
    auto sorted = products.sort("price", false);
    sorted.display();
    std::cout << "\n";
    
    // Aggregations
    std::cout << "Aggregations:\n";
    std::cout << "  Average price: $" << products.mean("price").mean() << "\n";
    std::cout << "  Total stock: " << products.sum("stock").sum() << "\n";
    std::cout << "  Min rating: " << products.min("rating").min() << "\n\n";
    
    // ============================================================
    // Example 3: Archive (3D) - Dictionary of Catalogs
    // ============================================================
    print_separator("3. Archive (3D) - Dictionary of Catalogs");
    
    Archive store("online_store");
    
    std::cout << "Creating multi-catalog archive:\n\n";
    
    // Add products catalog
    store["products"]["id"] = Feed(pluss::value({1.0, 2.0, 3.0}), "id");
    store["products"]["name"] = Feed(pluss::make_note({"Laptop", "Phone", "Tablet"}), "name");
    store["products"]["price"] = Feed(pluss::value({1299.99, 899.99, 499.99}), "price");
    
    // Add customers catalog
    store["customers"]["id"] = Feed(pluss::value({101.0, 102.0, 103.0}), "id");
    store["customers"]["name"] = Feed(pluss::make_note({"Alice", "Bob", "Charlie"}), "name");
    store["customers"]["email"] = Feed(pluss::make_note({"alice@email.com", "bob@email.com", "charlie@email.com"}), "email");
    
    // Add orders catalog
    store["orders"]["order_id"] = Feed(pluss::value({1001.0, 1002.0, 1003.0}), "order_id");
    store["orders"]["customer_id"] = Feed(pluss::value({101.0, 102.0, 103.0}), "customer_id");
    store["orders"]["product_id"] = Feed(pluss::value({1.0, 2.0, 3.0}), "product_id");
    store["orders"]["quantity"] = Feed(pluss::value({2.0, 1.0, 3.0}), "quantity");
    
    std::cout << "Archive structure:\n";
    std::cout << "  Number of catalogs: " << store.num_catalogs() << "\n";
    std::cout << "  Catalog names: ";
    for (const auto& name : store.keys()) {
        std::cout << name << " ";
    }
    std::cout << "\n\n";
    
    std::cout << "Products catalog:\n";
    store["products"].display();
    std::cout << "\n";
    
    std::cout << "Customers catalog:\n";
    store["customers"].display();
    std::cout << "\n";
    
    std::cout << "Orders catalog:\n";
    store["orders"].display();
    std::cout << "\n";
    
    // Archive-level operations
    std::cout << "Archive-level operations:\n";
    std::cout << "  Total rows across all catalogs: " << store.total_rows() << "\n";
    
    auto sizes = store.catalog_sizes();
    std::cout << "  Catalog sizes:\n";
    for (const auto& [name, size] : sizes) {
        std::cout << "    " << name << ": " << size << " rows\n";
    }
    std::cout << "\n";
    
    // ============================================================
    // Example 4: I/O Operations
    // ============================================================
    print_separator("4. I/O Operations");
    
    std::cout << "CSV I/O:\n\n";
    
    // Load catalog from CSV (example - file must exist)
    // auto loaded = Catalog::load("csv", "products.csv");
    // std::cout << "  Loaded catalog from products.csv\n";
    // std::cout << "  Rows: " << loaded.size() << ", Fields: " << loaded.num_fields() << "\n\n";
    
    std::cout << "  Note: Use Catalog::load(\"csv\", filename) to load CSV files\n";
    std::cout << "  Note: Use Archive::load_collection() or load_directory() for archives\n\n";
    
    // ============================================================
    // Example 5: Comparison with Thread-First (Table module)
    // ============================================================
    print_separator("5. Feed-First vs Thread-First");
    
    std::cout << "Handle module (Feed-First) vs Table module (Thread-First):\n\n";
    
    std::cout << "┌─────────────────────────────────────────────────────────────┐\n";
    std::cout << "│ Dimension │ Feed-First (Handle) │ Thread-First (Table)    │\n";
    std::cout << "├─────────────────────────────────────────────────────────────┤\n";
    std::cout << "│ 1D        │ Feed                │ Thread                  │\n";
    std::cout << "│ 2D        │ Catalog             │ DataFrame               │\n";
    std::cout << "│ 3D        │ Archive             │ Atlas                   │\n";
    std::cout << "├─────────────────────────────────────────────────────────────┤\n";
    std::cout << "│ Focus     │ Field operations    │ Row operations          │\n";
    std::cout << "│ Storage   │ Dictionary-based    │ Columnar/Row-based      │\n";
    std::cout << "│ Use Case  │ Document stores     │ Tabular analytics       │\n";
    std::cout << "└─────────────────────────────────────────────────────────────┘\n\n";
    
    std::cout << "When to use Feed-First (Handle):\n";
    std::cout << "  ✅ Field-oriented operations\n";
    std::cout << "  ✅ Flexible schema (add/remove fields easily)\n";
    std::cout << "  ✅ Dictionary-like access patterns\n";
    std::cout << "  ✅ Document-style data\n\n";
    
    std::cout << "When to use Thread-First (Table):\n";
    std::cout << "  ✅ Row-oriented operations\n";
    std::cout << "  ✅ Fixed schema analytics\n";
    std::cout << "  ✅ SQL-like operations\n";
    std::cout << "  ✅ Tabular data analysis\n\n";
    
    // ============================================================
    // Summary
    // ============================================================
    print_separator("Summary");
    
    std::cout << "Feed-First Design Philosophy:\n\n";
    std::cout << "  Feed (1D):\n";
    std::cout << "    • Homogeneous fields (Series, Note, or Timeline)\n";
    std::cout << "    • Operations: filter, map, reduce, take, sort\n";
    std::cout << "    • Aggregations: sum, mean, min, max, count\n\n";
    
    std::cout << "  Catalog (2D):\n";
    std::cout << "    • Dictionary of Feeds\n";
    std::cout << "    • Operations: filter, sort, group_by, merge\n";
    std::cout << "    • Field management: add, remove, rename\n\n";
    
    std::cout << "  Archive (3D):\n";
    std::cout << "    • Dictionary of Catalogs\n";
    std::cout << "    • Operations: filter_all, transform_all, aggregate_all\n";
    std::cout << "    • Multi-catalog management\n\n";
    
    std::cout << "Key Principles:\n";
    std::cout << "  1. Feed is homogeneous (all elements same type)\n";
    std::cout << "  2. Catalog is dictionary-based (flexible schema)\n";
    std::cout << "  3. Archive is collection of Catalogs\n";
    std::cout << "  4. Field-oriented operations are first-class\n";
    std::cout << "  5. Complements Thread-First (Table module)\n\n";
    
    return 0;
}
