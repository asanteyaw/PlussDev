/**
 * @file catalog_ex.cpp
 * @brief Catalog (2D) - Dictionary of Feeds Examples
 * 
 * Demonstrates Catalog operations (dictionary-based table).
 * Catalog is analogous to DataFrame in the Table module.
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
    print_separator("Catalog (2D) - Dictionary of Feeds");
    
    std::cout << "Catalog is a dictionary-based collection of Feeds:\n";
    std::cout << "  Feed (1D)      → Homogeneous field\n";
    std::cout << "  Catalog (2D)   → Dictionary of Feeds (Feed-First)\n";
    std::cout << "  Archive (3D)   → Dictionary of Catalogs\n\n";
    
    // ============================================================
    // Example 1: Creating a Catalog
    // ============================================================
    print_separator("1. Creating and Populating a Catalog");
    
    Catalog products("products");
    std::cout << "Created catalog: " << products.name() << "\n";
    std::cout << "Initial size: " << products.num_fields() << " fields\n\n";
    
    // Add fields to the catalog using Series (numeric data)
    products["id"] = Feed(pluss::value({1.0, 2.0, 3.0, 4.0}), "id");
    products["price"] = Feed(pluss::value({1299.99, 899.99, 499.99, 349.99}), "price");
    products["stock"] = Feed(pluss::value({45.0, 120.0, 75.0, 30.0}), "stock");
    products["rating"] = Feed(pluss::value({4.5, 4.7, 4.3, 4.6}), "rating");
    
    // Add text fields using Note
    products["name"] = Feed(pluss::make_note({"Laptop Pro 15", "Smartphone X", "Tablet Plus", "4K Monitor 27\""}), "name");
    products["category"] = Feed(pluss::make_note({"electronics", "electronics", "electronics", "electronics"}), "category");
    
    std::cout << "Added fields to catalog\n";
    std::cout << "Catalog now has " << products.num_fields() << " fields and " 
              << products.size() << " rows\n\n";
    
    std::cout << "Field names: ";
    for (const auto& key : products.keys()) {
        std::cout << key << " ";
    }
    std::cout << "\n\n";
    
    // Display the catalog
    std::cout << "Catalog display:\n";
    products.display();
    std::cout << "\n";
    
    // ============================================================
    // Example 2: Accessing and Querying Data
    // ============================================================
    print_separator("2. Accessing and Querying Data");
    
    // Access a single field
    std::cout << "Product names:\n";
    auto names = products["name"];
    names.display();
    std::cout << "\n";
    
    std::cout << "Product prices:\n";
    auto prices = products["price"];
    prices.display();
    std::cout << "\n";
    
    // Check if field exists
    std::cout << "Checking field existence:\n";
    std::cout << "  has('price'): " << (products.has("price") ? "yes" : "no") << "\n";
    std::cout << "  has('brand'): " << (products.has("brand") ? "yes" : "no") << "\n\n";
    
    // ============================================================
    // Example 3: Filtering
    // ============================================================
    print_separator("3. Filtering Rows");
    
    std::cout << "Filter products with price > 500:\n";
    auto price_series = products["price"].as_series();
    auto mask = price_series.gt(500.0);
    auto expensive = products.filter(mask);
    expensive.display();
    std::cout << "\n";
    
    std::cout << "Filter products with stock < 50:\n";
    auto stock_series = products["stock"].as_series();
    auto low_stock_mask = stock_series.lt(50.0);
    auto low_stock = products.filter(low_stock_mask);
    low_stock.display();
    std::cout << "\n";
    
    // ============================================================
    // Example 4: Sorting
    // ============================================================
    print_separator("4. Sorting");
    
    std::cout << "Sort by price (ascending):\n";
    auto sorted_asc = products.sort("price", true);
    sorted_asc.display();
    std::cout << "\n";
    
    std::cout << "Sort by price (descending):\n";
    auto sorted_desc = products.sort("price", false);
    sorted_desc.display();
    std::cout << "\n";
    
    std::cout << "Sort by multiple fields (rating desc, then price asc):\n";
    auto multi_sorted = products.sort_by({"rating", "price"}, {false, true});
    multi_sorted.display();
    std::cout << "\n";
    
    // ============================================================
    // Example 5: Slicing and Indexing
    // ============================================================
    print_separator("5. Slicing and Indexing");
    
    std::cout << "First 2 rows (head):\n";
    products.head(2).display();
    std::cout << "\n";
    
    std::cout << "Last 2 rows (tail):\n";
    products.tail(2).display();
    std::cout << "\n";
    
    std::cout << "Rows 1-3 (slice):\n";
    products.slice(1, 3).display();
    std::cout << "\n";
    
    // ============================================================
    // Example 6: Grouping
    // ============================================================
    print_separator("6. Grouping");
    
    std::cout << "Group by category:\n";
    auto groups = products.group_by("category");
    std::cout << "Number of groups: " << groups.size() << "\n\n";
    
    for (const auto& [category, group_catalog] : groups) {
        std::cout << "Category: " << category << "\n";
        std::cout << "  Count: " << group_catalog.size() << " products\n";
        std::cout << "  Average price: $" << group_catalog.mean("price").mean() << "\n";
        std::cout << "  Total stock: " << group_catalog.sum("stock").sum() << "\n\n";
    }
    
    // ============================================================
    // Example 7: Field Operations
    // ============================================================
    print_separator("7. Field Operations");
    
    std::cout << "Select specific fields:\n";
    auto subset = products.select({"name", "price", "rating"});
    subset.display();
    std::cout << "\n";
    
    std::cout << "Drop fields:\n";
    auto dropped = products.drop({"stock", "category"});
    dropped.display();
    std::cout << "\n";
    
    std::cout << "Rename fields:\n";
    auto renamed = products.rename({{"name", "product_name"}, {"price", "unit_price"}});
    renamed.head(3).display();
    std::cout << "\n";
    
    // ============================================================
    // Example 8: Aggregations
    // ============================================================
    print_separator("8. Aggregations");
    
    std::cout << "Sum of stock:\n";
    auto total_stock = products.sum("stock");
    std::cout << "  Total: " << total_stock.sum() << "\n\n";
    
    std::cout << "Average price:\n";
    auto avg_price = products.mean("price");
    std::cout << "  Average: $" << avg_price.mean() << "\n\n";
    
    std::cout << "Min and Max rating:\n";
    auto min_rating = products.min("rating");
    auto max_rating = products.max("rating");
    std::cout << "  Min: " << min_rating.min() << "\n";
    std::cout << "  Max: " << max_rating.max() << "\n\n";
    
    std::cout << "Total count:\n";
    std::cout << "  " << products.count() << " rows\n\n";
    
    // ============================================================
    // Example 9: Merging Catalogs
    // ============================================================
    print_separator("9. Merging Catalogs");
    
    // Create another catalog
    Catalog suppliers("suppliers");
    suppliers["id"] = Feed(pluss::value({1.0, 2.0, 3.0}), "id");
    suppliers["supplier"] = Feed(pluss::make_note({"TechCorp", "MobileCo", "DisplayInc"}), "supplier");
    suppliers["contact"] = Feed(pluss::make_note({"tech@corp.com", "info@mobile.co", "sales@display.inc"}), "contact");
    
    std::cout << "Suppliers catalog:\n";
    suppliers.display();
    std::cout << "\n";
    
    std::cout << "Inner join on 'id':\n";
    auto inner_joined = products.merge(suppliers, "id", "inner");
    inner_joined.display();
    std::cout << "\n";
    
    std::cout << "Left join on 'id':\n";
    auto left_joined = products.merge(suppliers, "id", "left");
    left_joined.display();
    std::cout << "\n";
    
    // ============================================================
    // Example 10: Catalog Information
    // ============================================================
    print_separator("10. Catalog Information");
    
    std::cout << "Catalog info:\n";
    products.info();
    std::cout << "\n";
    
    std::cout << "Catalog as string:\n";
    std::cout << products.to_string() << "\n\n";
    
    // ============================================================
    // Summary
    // ============================================================
    print_separator("Summary");
    
    std::cout << "Catalog (2D) provides dictionary-based table operations:\n\n";
    std::cout << "  ✅ Field access: operator[], at(), has(), remove()\n";
    std::cout << "  ✅ Metadata: size(), num_fields(), keys(), empty()\n";
    std::cout << "  ✅ Indexing: head(), tail(), slice(), filter()\n";
    std::cout << "  ✅ Sorting: sort(), sort_by()\n";
    std::cout << "  ✅ Grouping: group_by() returns map<string, Catalog>\n";
    std::cout << "  ✅ Field ops: select(), drop(), rename()\n";
    std::cout << "  ✅ Aggregations: sum(), mean(), min(), max(), count()\n";
    std::cout << "  ✅ Merging: merge() with inner/left/right/outer joins\n";
    std::cout << "  ✅ Display: info(), display(), to_string()\n\n";
    
    std::cout << "Catalog complements DataFrame from the Table module:\n";
    std::cout << "  - Use DataFrame (Thread-First) for row-oriented operations\n";
    std::cout << "  - Use Catalog (Feed-First) for field-oriented operations\n\n";
    
    return 0;
}
