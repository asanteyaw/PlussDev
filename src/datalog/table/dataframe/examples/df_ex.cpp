#include <iostream>
#include <pluss/pluss.h>
#include <filesystem>
#include <pluss/datalog/table/dataframe/dataframe.h>
#include <pluss/datalog/table/thread/thread.h>

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
    df->describe();

    std::cout << "\n-- Head (5 rows) --\n";
    df->head(5);

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

    if (std::filesystem::exists(messy)) {
        std::cout << "\nLoading messy_data.csv to stress test CSV parser...\n";
        auto df2 = DataFrame::load("csv", messy);
        if (df2) {
            std::cout << "Loaded messy_data: columns=" << df2->columns().size() << " rows ~ " << (df2->values().empty() ? 0 : df2->values()[0].size()) << "\n";
            df2->info();
            df2->head(3);
            std::cout << "\n-- Describe messy_data (best-effort) --\n";
            df2->describe();
        } else {
            std::cout << "Failed to load messy_data.csv\n";
        }
    } else {
        std::cout << "messy_data.csv not found; skipping.\n";
    }

    return 0;
}
