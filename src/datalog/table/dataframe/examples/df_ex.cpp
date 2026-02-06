#include <iostream>
#include <filesystem>

#include <pluss/pluss.h>

using namespace pluss;
using namespace pluss::table;

int main(int argc, char** argv) {
    // When running from the `build/` directory the data files are one level up
    std::string bank = "../data/bank-full.csv";
    std::string messy = "../data/messy_data.csv";

    if (!std::filesystem::exists(bank)) {
        std::cerr << "Example data not found: " << bank << "\n";
        return 1;
    }

    std::cout << "Loading bank-full.csv...\n";
    auto df = DataFrame::load("csv", bank);
    if (!df) {
        std::cerr << "Failed to load " << bank << "\n";
        return 1;
    }
    std::cout << "Columns (" << df->columns().size() << "):\n";
    for (auto &c : df->columns()) std::cout << " - " << c << "\n";

    std::cout << "\n-- Info --\n";
    df->info();

    std::cout << "\n-- Describe (numeric summary) --\n";
    df->describe(0);  // Explicit format parameter to use display version

    std::cout << "\n-- Head (5 rows) --\n";
    df->head(5, 0);  // Explicit format parameter to use display version

    // save a small sample of the top rows
    auto head_df = df->iloc(0, std::min<int64_t>(5, static_cast<int64_t>(df->values().empty() ? 0 : df->values()[0].size())));
    std::string out_head = "../build/bank_head.csv";
    head_df->to_csv(out_head);
    std::cout << "Wrote head sample to: " << out_head << "\n";

    // Access a column
    if (!df->columns().empty()) {
        auto col0 = df->get_col(df->columns().front());
        std::cout << "First column is named: " << col0.name() << " size=" << col0.size() << "\n";
        if (col0.is_series()) std::cout << "-> underlying series dim=" << col0.as_series().dim() << "\n";
    }
    
    // Test 1: Display single column (Thread) - should be simple text with index
    std::cout << "\n-- Single Column Display (age) --\n";
    auto age_col = df->get_col("age");
    std::cout << "Thread: " << age_col.name() << " [" << age_col.size() << " rows]\n";
    // For now, show first 10 values
    if (age_col.is_series()) {
        auto s = age_col.as_series();
        auto sz = s.data().size(0);
        for (int64_t i = 0; i < std::min<int64_t>(10, sz); ++i) {
            std::cout << i << "  " << age_col.cell_value(i) << "\n";
        }
        if (sz > 10) {
            std::cout << "... (" << sz - 10 << " more rows)\n";
        }
    }
    
    // Test 2: Print whole DataFrame (like pandas print(df))
    std::cout << "\n-- Whole DataFrame (print(df)) --\n";
    pluss::table::io::print(std::cout, *df) << "\n";

    if (std::filesystem::exists(messy)) {
        std::cout << "\nLoading messy_data.csv to stress test CSV parser...\n";
        auto df2 = DataFrame::load("csv", messy);
        if (df2) {
            std::cout << "Loaded messy_data: columns=" << df2->columns().size() << " rows ~ " << (df2->values().empty() ? 0 : df2->values()[0].size()) << "\n";
            df2->info();
            std::cout << "\n-- Displaying head of 10 --\n";
            df2->head(10, 0);  // Explicit format parameter to use display version
            std::cout << "\n-- Describe messy_data (best-effort) --\n";
            df2->describe(0);  // Explicit format parameter to use display version
        } else {
            std::cout << "Failed to load messy_data.csv\n";
        }
    } else {
        std::cout << "messy_data.csv not found; skipping.\n";
    }

    return 0;
}
