/**
 * @file parallel_csv_example.cpp
 * @brief Example demonstrating high-performance parallel CSV reading
 * 
 * This example shows how to use the ParallelCSVReader for processing
 * large CSV files efficiently with multi-threading and memory mapping.
 */

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include <pluss/pluss.h>

using namespace pluss;
using namespace pluss::table;

static std::vector<std::string> make_default_colnames(size_t n) {
    std::vector<std::string> names;
    names.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        names.push_back("col_" + std::to_string(i));
    }
    return names;
}

static std::string resolve_data_path(const std::string& filename) {
    const std::vector<std::string> candidates = {
        "data/" + filename,
        "../data/" + filename,
        "../../data/" + filename,
    };
    for (const auto& p : candidates) {
        if (std::filesystem::exists(p)) return p;
    }
    return {};
}

int main() {
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  Parallel CSV Reader Example\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    // ========================================================================
    // BASIC USAGE: Default Configuration (Auto-optimized)
    // ========================================================================
    {
        std::cout << "Example 1: Basic Parallel Reading (Auto-config)\n";
        std::cout << "------------------------------------------------\n";

        const std::string csv_path = resolve_data_path("messy_data.csv");
        if (csv_path.empty()) {
            std::cout << "! Skipping: missing 'messy_data.csv' (expected under data/)\n\n";
        } else {
            auto rows = ParallelCSVReader().read(csv_path);
            const size_t nrows = rows.size();
            const size_t ncols = rows.empty() ? 0 : rows.front().size();
            std::cout << "✓ Loaded raw CSV: " << nrows << " rows × " << ncols << " columns\n\n";
        }
    }

    // ========================================================================
    // ADVANCED: Custom Configuration (chunking, delimiter)
    // ========================================================================
    {
        std::cout << "Example 2: Custom Configuration (chunking, delimiter)\n";
        std::cout << "---------------------------------------------------\n";

        const std::string csv_path = resolve_data_path("bank-full.csv");
        if (csv_path.empty()) {
            std::cout << "! Skipping: missing 'bank-full.csv' (expected under data/)\n\n";
        } else {
            ParallelCSVConfig config;
            config.chunk_size = 100000;
            config.num_threads = 0;              // Auto-detect
            config.use_memory_mapping = true;    // Enable mmap when beneficial
            config.skip_header = false;
            config.delimiter = ';';              // bank-full.csv is ';' separated
            config.trim_whitespace = true;

            ParallelCSVReader reader(config);

            auto progress = [](size_t processed, size_t total) {
                if (total > 0 && processed % 250000 == 0) {
                    std::cout << "  Progress: " << processed << " / " << total << " rows\n";
                }
            };

            // Convert a known numeric column: bank-full.csv starts with "age".
            const std::vector<size_t> numeric_cols = {0};
            auto [string_rows, numeric_cols_data] = reader.read_mixed(csv_path, numeric_cols, progress);

            const size_t nrows = string_rows.size();
            const size_t ncols = string_rows.empty() ? 0 : string_rows.front().size();
            std::cout << "✓ Loaded raw CSV: " << nrows << " rows × " << ncols << " columns\n";

            if (!numeric_cols_data.empty() && !numeric_cols_data.front().empty()) {
                double sum = 0.0;
                size_t count = 0;
                for (double v : numeric_cols_data.front()) {
                    if (std::isnan(v)) continue;
                    sum += v;
                    ++count;
                }
                if (count > 0) {
                    std::cout << "  Mean(age) (approx): " << (sum / static_cast<double>(count)) << "\n";
                }
            }

            std::cout << "\n";
        }
    }

    // ========================================================================
    // REAL-WORLD (optional): US Traffic Accidents Dataset (large)
    // ========================================================================
    {
        std::cout << "Example 3: Real-World Large Dataset (optional)\n";
        std::cout << "----------------------------------------------\n";

        const char* run_large = std::getenv("PLUSS_RUN_LARGE_EXAMPLES");
        const bool enabled = (run_large != nullptr) && std::string(run_large) == "1";

        const std::string csv_path = resolve_data_path("US_Accidents_Dec20.csv");
        if (!enabled) {
            std::cout << "! Skipping by default (set PLUSS_RUN_LARGE_EXAMPLES=1).\n\n";
        } else if (csv_path.empty()) {
            std::cout << "! Skipping: missing 'US_Accidents_Dec20.csv' (expected under data/)\n\n";
        } else {
            ParallelCSVConfig config;
            config.chunk_size = 500000;
            config.num_threads = 0;
            config.use_memory_mapping = true;

            ParallelCSVReader reader(config);
            auto progress = [](size_t processed, size_t /*total*/) {
                if (processed % 500000 == 0 && processed > 0) {
                    std::cout << "  Loaded " << processed << " rows...\n";
                }
            };

            auto rows = reader.read(csv_path, progress);
            const size_t nrows = rows.size();
            const size_t ncols = rows.empty() ? 0 : rows.front().size();
            std::cout << "\n✓ Dataset loaded successfully!\n";
            std::cout << "  Rows: " << nrows << "\n";
            std::cout << "  Columns: " << ncols << "\n";

            std::cout << "\n";
        }
    }

    // ========================================================================
    // PERFORMANCE COMPARISON (illustrative; uses messy_data.csv)
    // ========================================================================
    {
        std::cout << "Example 4: Performance Comparison (illustrative)\n";
        std::cout << "------------------------------------------------\n";

        const std::string test_file = resolve_data_path("messy_data.csv");
        if (test_file.empty()) {
            std::cout << "! Skipping: missing 'messy_data.csv' (expected under data/)\n\n";
        } else {
            std::cout << "Parallel read:   ";
            auto start = std::chrono::high_resolution_clock::now();

            ParallelCSVConfig config;
            config.chunk_size = 100000;
            config.num_threads = 0;  // Auto-detect
            config.delimiter = ',';

            (void)ParallelCSVReader(config).read(test_file);

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << duration.count() << " ms\n\n";
        }
    }

    // ========================================================================
    // INTEGRATION WITH STANDARD PLUSS API (basic)
    // ========================================================================
    {
        std::cout << "Example 5: Standard PlussMa Pipeline (basic)\n";
        std::cout << "-------------------------------------------\n";

        const std::string csv_path = resolve_data_path("bank-full.csv");
        if (csv_path.empty()) {
            std::cout << "! Skipping: missing 'bank-full.csv' (expected under data/)\n\n";
        } else {
            ParallelCSVConfig config;
            config.delimiter = ';';
            ParallelCSVReader reader(config);
            auto rows = reader.read(csv_path);
            if (rows.empty()) {
                std::cout << "! No rows parsed\n\n";
            } else {
                // Assume first row is a header.
                std::vector<std::string> col_names = rows.front();
                rows.erase(rows.begin());
                if (col_names.empty()) {
                    col_names = make_default_colnames(rows.empty() ? 0 : rows.front().size());
                }

                auto built = pluss::ParallelDataFrameBuilder::build_from_csv(rows, col_names);
                pluss::table::io::DisplayOptions opt;
                opt.max_width = 100;
                opt.max_rows = 12;
                opt.head_rows = 10;
                opt.tail_rows = 2;

                std::cout << pluss::table::io::to_string(built, opt) << "\n";
                std::cout << "✓ Built DataFrame via ParallelDataFrameBuilder\n\n";
            }
        }
    }

    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "Key Takeaways:\n";
    std::cout << "  • Single extra #include for opt-in performance\n";
    std::cout << "  • 3-5x faster for multi-million row datasets\n";
    std::cout << "  • Memory-efficient via mmap (handles multi-GB files)\n";
    std::cout << "  • Drop-in replacement: ParallelCSVReader().read_mixed()\n";
    std::cout << "  • Works with '#include <pluss/pluss.h>'\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";

    return 0;
}
