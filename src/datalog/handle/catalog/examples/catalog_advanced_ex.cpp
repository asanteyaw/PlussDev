/**
 * @file catalog_advanced_ex.cpp
 * @brief Advanced Catalog operations with vectorized methods
 * 
 * Demonstrates the rich vectorized functionality of Catalog including:
 * - Filtering, sorting, grouping
 * - Join operations
 * - Aggregations
 * - Text processing
 */

#include <pluss/pluss.h>
#include <iostream>

using namespace pluss::handle;

void example_sorting_and_grouping() {
    std::cout << "\n=== Example: Sorting and GroupBy ===\n";
    
    Catalog sales("sales");
    
    sales["order_id"] = Feed(pluss::value({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}), "order_id");
    sales["product"] = Feed(pluss::make_note({"Laptop", "Mouse", "Laptop", "Keyboard", "Mouse", "Laptop"}), "product");
    sales["quantity"] = Feed(pluss::value({1.0, 2.0, 1.0, 3.0, 1.0, 2.0}), "quantity");
    sales["revenue"] = Feed(pluss::value({999.0, 59.98, 999.0, 239.97, 29.99, 1999.98}), "revenue");
    
    std::cout << "Original Sales Data:\n";
    sales.display();
    
    // Sort by revenue
    std::cout << "\n--- Sorted by revenue (descending) ---\n";
    auto sorted = sales.sort("revenue", false);
    sorted.display();
    
    // Group by product
    std::cout << "\n--- Grouped by product ---\n";
    auto groups = sales.group_by("product");
    
    for (const auto& [product, group] : groups) {
        std::cout << "\nProduct: " << product << "\n";
        std::cout << "Count: " << group.count() << "\n";
        std::cout << "Total Revenue: $" << group.sum("revenue").sum() << "\n";
        std::cout << "Avg Revenue: $" << group.mean("revenue").mean() << "\n";
    }
}

void example_joins() {
    std::cout << "\n=== Example: Join Operations ===\n";
    
    Catalog users("users");
    users["user_id"] = Feed(pluss::value({1.0, 2.0, 3.0, 4.0}), "user_id");
    users["username"] = Feed(pluss::make_note({"alice", "bob", "charlie", "diana"}), "username");
    users["city"] = Feed(pluss::make_note({"NYC", "LA", "Chicago", "Boston"}), "city");
    
    Catalog orders("orders");
    orders["order_id"] = Feed(pluss::value({101.0, 102.0, 103.0, 104.0}), "order_id");
    orders["user_id"] = Feed(pluss::value({1.0, 2.0, 2.0, 3.0}), "user_id");
    orders["amount"] = Feed(pluss::value({50.0, 75.0, 120.0, 200.0}), "amount");
    
    std::cout << "Users:\n";
    users.display();
    
    std::cout << "\nOrders:\n";
    orders.display();
    
    // Inner join
    std::cout << "\n--- Inner Join on user_id ---\n";
    auto joined = users.merge(orders, "user_id", "inner");
    joined.display();
}

void example_text_processing() {
    std::cout << "\n=== Example: Text Processing ===\n";
    
    Catalog documents("documents");
    
    documents["doc_id"] = Feed(pluss::value({1.0, 2.0, 3.0}), "doc_id");
    documents["text"] = Feed(pluss::make_note({
        "Machine learning algorithms power modern AI systems.",
        "Natural language processing enables text understanding.",
        "Deep learning models require large datasets for training."
    }), "text");
    
    std::cout << "Original Documents:\n";
    documents.display();
    
    // Add text features
    std::cout << "\n--- With Text Features ---\n";
    auto with_features = documents.add_text_features("text");
    with_features.display();
    
    // Filter by keyword
    std::cout << "\n--- Filter by 'learning' ---\n";
    auto filtered = documents.filter_by_text("text", "learning");
    filtered.display();
    
    // Text preprocessing
    std::cout << "\n--- Preprocessed Text ---\n";
    auto preprocessed = documents.text_preprocess("text");
    preprocessed.display();
}

void example_aggregations() {
    std::cout << "\n=== Example: Aggregations ===\n";
    
    Catalog metrics("metrics");
    
    metrics["timestamp"] = Feed(pluss::value({1.0, 2.0, 3.0, 4.0, 5.0}), "timestamp");
    metrics["cpu"] = Feed(pluss::value({45.2, 67.8, 55.3, 89.1, 72.4}), "cpu");
    metrics["memory"] = Feed(pluss::value({60.5, 75.2, 68.9, 85.7, 79.3}), "memory");
    
    metrics.display();
    
    std::cout << "\n--- Statistical Summary ---\n";
    std::cout << "CPU Sum: " << metrics.sum("cpu").sum() << "\n";
    std::cout << "CPU Mean: " << metrics.mean("cpu").mean() << "\n";
    std::cout << "CPU Min: " << metrics.min("cpu").min() << "\n";
    std::cout << "CPU Max: " << metrics.max("cpu").max() << "\n";
    std::cout << "Total Records: " << metrics.count() << "\n";
}

int main() {
    std::cout << "=========================================\n";
    std::cout << "  Advanced Catalog Examples\n";
    std::cout << "=========================================\n";
    
    try {
        example_sorting_and_grouping();
        example_joins();
        example_text_processing();
        example_aggregations();
        
        std::cout << "\n=========================================\n";
        std::cout << "  All Examples Completed Successfully!\n";
        std::cout << "=========================================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
