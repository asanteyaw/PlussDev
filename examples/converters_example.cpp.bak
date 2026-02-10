#include <pluss/pluss.h>
#include <pluss/datalog/converters.h>
#include <iostream>
#include <iomanip>

using namespace pluss;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(70, '=') << "\n\n";
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   PlussMa Conversion Utilities: tablize() & handelize()        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";

    // ========== Example 1: Feed ↔ Thread ==========
    print_separator("1. Feed ↔ Thread Conversion (Trivial)");
    
    auto scores = pluss::value({85.5, 92.0, 78.5, 88.0, 95.5});
    handle::Feed feed(scores, "test_scores");
    
    std::cout << "Original Feed: " << feed.name() << "\n";
    std::cout << "  Size: " << feed.size() << "\n";
    std::cout << "  Mean: " << feed.mean() << "\n\n";
    
    // Convert to Thread
    auto thread = converters::tablize(feed);
    std::cout << "After tablize → Thread: " << thread.name() << "\n";
    std::cout << "  Size: " << thread.size() << "\n\n";
    
    // Convert back to Feed
    auto feed2 = converters::handelize(thread);
    std::cout << "After handelize → Feed: " << feed2.name() << "\n";
    std::cout << "  Size: " << feed2.size() << "\n";
    std::cout << "  Mean: " << feed2.mean() << "\n";

    // ========== Example 2: Catalog → DataFrame ==========
    print_separator("2. Catalog → DataFrame: Configuration to Table");
    
    handle::Catalog model_config("ml_experiments");
    
    // Experiment 1
    model_config["exp_001"] = handle::Feed(pluss::value({0.87}), "accuracy");
    
    // Experiment 2  
    model_config["exp_002"] = handle::Feed(pluss::value({0.91}), "accuracy");
    
    // Experiment 3
    model_config["exp_003"] = handle::Feed(pluss::value({0.89}), "accuracy");
    
    std::cout << "Original Catalog: " << model_config.name() << "\n";
    std::cout << "  Num entries: " << model_config.num_fields() << "\n";
    std::cout << "  Keys: ";
    for (const auto& key : model_config.keys()) {
        std::cout << key << " ";
    }
    std::cout << "\n\n";
    
    // Check if can convert
    if (converters::can_tablize(model_config)) {
        std::cout << "✓ Catalog can be tablized\n\n";
        
        // Convert to DataFrame
        auto exp_df = converters::tablize(model_config, "experiment_id");
        std::cout << "Converted to DataFrame:\n";
        auto exp_df_ptr = std::make_shared<table::DataFrame>(exp_df);
        exp_df_ptr->head(10, 0);
        
        // Now we can do DataFrame operations
        std::cout << "\nSorted by accuracy (descending):\n";
        auto sorted = exp_df_ptr->sort_values("accuracy", false);
        sorted->head(10, 0);
    }

    // ========== Example 3: DataFrame → Catalog ==========
    print_separator("3. DataFrame → Catalog: User Data to Dictionary");
    
    // Create a DataFrame with user data
    auto users_df = std::make_shared<table::DataFrame>();
    std::vector<Chord> ids = {Chord("alice"), Chord("bob"), Chord("charlie")};
    std::vector<double> ages = {28.0, 35.0, 42.0};
    std::vector<double> user_scores = {85.5, 92.0, 78.5};
    
    users_df = users_df->set_col("user_id", table::Thread(make_note(ids), "user_id"));
    users_df = users_df->set_col("age", table::Thread(pluss::value(ages), "age"));
    users_df = users_df->set_col("score", table::Thread(pluss::value(user_scores), "score"));
    
    std::cout << "Original DataFrame:\n";
    users_df->head(10, 0);
    
    // Check if can convert
    if (converters::can_handelize(*users_df, "user_id")) {
        std::cout << "\n✓ DataFrame can be handelized\n\n";
        
        // Convert to Catalog
        auto users_catalog = converters::handelize(*users_df, "user_id");
        std::cout << "Converted to Catalog: " << users_catalog.name() << "\n";
        std::cout << "  Num entries: " << users_catalog.num_fields() << "\n\n";
        
        // Fast key-based lookup (O(1))
        std::cout << "Looking up user 'bob':\n";
        auto bob_data = users_catalog["bob"];
        std::cout << "  Name: " << bob_data.name() << "\n";
        std::cout << "  Value: " << bob_data.to_string() << "\n";
    }

    // ========== Example 4: Hybrid Workflow ==========
    print_separator("4. Hybrid Workflow: Analysis + Lookup");
    
    // Start with table for analysis
    auto sales_df = std::make_shared<table::DataFrame>();
    std::vector<Chord> products = {Chord("laptop"), Chord("mouse"), Chord("keyboard"), Chord("monitor")};
    std::vector<double> revenues = {1200.0, 25.0, 75.0, 350.0};
    std::vector<double> quantities = {10.0, 150.0, 80.0, 15.0};
    
    sales_df = sales_df->set_col("product", table::Thread(make_note(products), "product"));
    sales_df = sales_df->set_col("revenue", table::Thread(pluss::value(revenues), "revenue"));
    sales_df = sales_df->set_col("quantity", table::Thread(pluss::value(quantities), "quantity"));
    
    std::cout << "Sales Data (DataFrame - for analysis):\n";
    sales_df->head(10, 0);
    
    // Analyze: filter high-value products
    auto revenue_thread = sales_df->get_col("revenue");
    auto revenue_series = revenue_thread.as_series();
    auto mask_tensor = revenue_series.gt(100.0);
   //  auto high_value_mask = pluss::from_torch(mask_tensor);
    auto high_value = sales_df->loc(mask_tensor);
    std::cout << "\nHigh-value products (revenue > $100):\n";
    high_value->head(10, 0);
    
    // Convert to Catalog for fast serving
    auto product_catalog = converters::handelize(*high_value, "product");
    
    std::cout << "\nConverted to Catalog for O(1) lookups:\n";
    std::cout << "  Quick lookup for 'laptop':\n";
    auto laptop_data = product_catalog["laptop"];
    std::cout << "    Field: " << laptop_data.name() << "\n";
    std::cout << "    Data: " << laptop_data.to_string() << "\n";

    // ========== Example 5: Archive ↔ Atlas ==========
    print_separator("5. Archive ↔ Atlas: Multi-table Conversion");
    
    // Create DataFrames using set_col
    auto customers = std::make_shared<table::DataFrame>();
    std::vector<Chord> cust_ids = {Chord("cust_001"), Chord("cust_002")};
    std::vector<double> cust_values = {5000.0, 3500.0};
    customers = customers->set_col("customer_id", table::Thread(make_note(cust_ids), "customer_id"));
    customers = customers->set_col("lifetime_value", table::Thread(pluss::value(cust_values), "lifetime_value"));
    
    auto products_df = std::make_shared<table::DataFrame>();
    std::vector<Chord> prod_ids = {Chord("prod_A"), Chord("prod_B")};
    std::vector<double> prices = {99.99, 149.99};
    products_df = products_df->set_col("product_id", table::Thread(make_note(prod_ids), "product_id"));
    products_df = products_df->set_col("price", table::Thread(pluss::value(prices), "price"));
    
    // Create Atlas
    std::unordered_map<std::string, std::shared_ptr<table::DataFrame>> frames;
    frames["customers"] = customers;
    frames["products"] = products_df;
    table::Atlas company_atlas(frames);
    
    std::cout << "Original Atlas\n";
    std::cout << "  Tables: " << company_atlas.ndepth() << "\n\n";
    
    // Convert entire Atlas to Archive
    std::cout << "Note: Atlas → Archive conversion processes each DataFrame independently\n";
    std::cout << "  Each DataFrame with proper key column becomes a Catalog\n";

    // ========== Summary ==========
    print_separator("Summary: When to Use What");
    
    std::cout << "tablize() - Convert Handle to Table:\n";
    std::cout << "  • Feed → Thread: Direct conversion\n";
    std::cout << "  • Catalog → DataFrame: Keys become a column, enables analysis\n";
    std::cout << "  • Archive → Atlas: Batch conversion\n";
    std::cout << "  USE CASE: Need to perform statistical analysis on dictionary data\n\n";
    
    std::cout << "handelize() - Convert Table to Handle:\n";
    std::cout << "  • Thread → Feed: Direct conversion\n";
    std::cout << "  • DataFrame → Catalog: Row ID becomes key, enables O(1) lookup\n";
    std::cout << "  • Atlas → Archive: Batch conversion\n";
    std::cout << "  USE CASE: Need fast key-based lookups for serving/production\n\n";
    
    std::cout << "Validation:\n";
    std::cout << "  • can_tablize(): Check if all Feeds have same length\n";
    std::cout << "  • can_handelize(): Check for valid, unique key column\n\n";

    std::cout << "\n✅ Conversion utilities demonstration complete!\n\n";
    
    return 0;
}
