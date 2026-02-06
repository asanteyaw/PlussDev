/**
 * @file parallel_csv_simple_test.cpp
 * @brief Simple demonstration of ParallelCSVReader usage
 */

#include <iostream>
#include <chrono>

#include <pluss/pluss.h>

using namespace pluss;
using namespace pluss::table;

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         Parallel CSV Reader - Simple Test                   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";

    const std::string csv_path = "/Users/yawasante/software/cxx/PlussMa/data/US_Accidents_Dec20.csv";

    // ========================================================================
    // DEMO: Parallel CSV Reader with Configuration
    // ========================================================================
    
    std::cout << "Configuration:\n";
    std::cout << "  • File: US_Accidents_Dec20.csv\n";
    std::cout << "  • Size: ~1.5GB\n";
    std::cout << "  • Expected rows: ~4.2M\n";
    std::cout << "  • Chunk size: 500K rows\n";
    std::cout << "  • Threads: Auto-detect\n\n";

    ParallelCSVConfig config;
    config.chunk_size = 500000;
    config.num_threads = 0;  // Auto-detect
    config.use_memory_mapping = true;

    ParallelCSVReader reader(config);

    std::cout << "Loading dataset with parallel reader...\n";
    
    auto start = std::chrono::high_resolution_clock::now();

    // Progress callback
    auto progress = [](size_t processed, size_t total) {
        if (processed % 500000 == 0) {
            std::cout << "  Processed: " << processed << " rows...\n";
        }
    };

    // Read CSV in parallel
    auto raw_data = reader.read(csv_path, progress);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "\n✓ Loading complete!\n";
    std::cout << "  Rows loaded: " << raw_data.size() << "\n";
    std::cout << "  Columns: " << (raw_data.empty() ? 0 : raw_data[0].size()) << "\n";
    std::cout << "  Time: " << duration.count() << " ms (" 
              << (duration.count() / 1000.0) << " sec)\n\n";

    // ========================================================================
    // Build DataFrame (demonstration - in production use read_mixed())
    // ========================================================================
    
    std::cout << "Note: For full DataFrame integration, use read_mixed():\n";
    std::cout << "  auto df = ParallelCSVReader(config).read_mixed(csv_path);\n\n";

    // Show sample data
    if (!raw_data.empty()) {
        std::cout << "Sample data (first row):\n";
        for (size_t i = 0; i < std::min(size_t(5), raw_data[0].size()); ++i) {
            std::cout << "  [" << i << "] " << raw_data[0][i] << "\n";
        }
    }

    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║ Performance Comparison:                                      ║\n";
    std::cout << "║   Sequential CSV parsing: ~57 seconds                        ║\n";
    std::cout << "║   Parallel CSV parsing:   ~" << (duration.count() / 1000.0) << " seconds                          ║\n";
    std::cout << "║   Speedup: ~" << (57.0 / (duration.count() / 1000.0)) << "x                                         ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";

    return 0;
}
