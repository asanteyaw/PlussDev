#include <iostream>

#include <torch/torch.h>

#include <pluss/pluss.h>

int main() {
    using pluss::table::DataFrame;
    using pluss::table::Thread;
    using pluss::table::io::DisplayOptions;

    std::cout << "=== DataFrame Display Demo ===\n\n";

    // Create a sample DataFrame with various data types
    auto df = std::make_shared<DataFrame>();

    // Numeric columns
    std::vector<int64_t> ids;
    std::vector<double> prices;
    std::vector<int64_t> quantities;
    for (int i = 0; i < 100; ++i) {
        ids.push_back(1000 + i);
        prices.push_back(19.99 + i * 2.5);
        quantities.push_back(10 + (i % 50));
    }

    df = df->set_col("ProductID", Thread(pluss::from_torch(torch::series(ids)), "ProductID"));
    df = df->set_col("Price", Thread(pluss::from_torch(torch::series(prices)), "Price"));
    df = df->set_col("Quantity", Thread(pluss::from_torch(torch::series(quantities)), "Quantity"));

    // Text column
    std::vector<pluss::Chord> products;
    std::vector<pluss::Chord> categories;
    for (int i = 0; i < 100; ++i) {
        products.emplace_back("Product-" + std::to_string(i) + " with a very long descriptive name that will be truncated");
        categories.emplace_back((i % 3 == 0) ? "Electronics" : (i % 3 == 1) ? "Furniture" : "Clothing");
    }
    df = df->set_col("ProductName", Thread(pluss::make_note(products), "ProductName"));
    df = df->set_col("Category", Thread(pluss::make_note(categories), "Category"));

    std::cout << "1. Default display (auto-fit width, truncate rows):\n";
    std::cout << *df << "\n\n";

    std::cout << "2. Head (first 5 rows only):\n";
    df->head(5, 0);  // Explicit format parameter to use display version
    std::cout << "\n";

    std::cout << "3. Tail (last 5 rows only):\n";
    df->tail(5, 0);  // Explicit format parameter to use display version
    std::cout << "\n";

    std::cout << "4. Info (metadata):\n";
    df->info();
    std::cout << "\n";

    std::cout << "5. Describe (statistical summary):\n";
    df->describe(0);  // Explicit format parameter to use display version
    std::cout << "\n";

    std::cout << "6. Custom display options (narrow width, more rows):\n";
    DisplayOptions opt;
    opt.max_width = 80;
    opt.max_rows = 15;
    opt.head_rows = 10;
    opt.tail_rows = 5;
    opt.show_summary = true;
    std::cout << pluss::table::io::to_string(*df, opt) << "\n\n";

    std::cout << "7. Very narrow display (forces column truncation):\n";
    opt.max_width = 50;
    opt.max_rows = 8;
    opt.head_rows = 4;
    opt.tail_rows = 4;
    std::cout << pluss::table::io::to_string(*df, opt) << "\n\n";

    std::cout << "8. Column limit (max 3 columns visible):\n";
    opt.max_width = 120;
    opt.max_cols = 3;
    opt.max_rows = 5;
    opt.head_rows = 5;
    opt.tail_rows = 0;
    std::cout << pluss::table::io::to_string(*df, opt) << "\n\n";

    std::cout << "=== Demo Complete ===\n";
    return 0;
}
