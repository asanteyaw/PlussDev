/**
 * @file archive_ex.cpp
 * @brief Archive (3D) - Collection of Catalogs Examples
 * 
 * Demonstrates Archive operations with the actual API.
 * Archive is analogous to Atlas in the Table module.
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
    print_separator("Archive (3D) - Collection of Catalogs");
    
    std::cout << "Archive is a dictionary-based collection of Catalogs:\n";
    std::cout << "  Feed (1D)      → Homogeneous field\n";
    std::cout << "  Catalog (2D)   → Dictionary of Feeds\n";
    std::cout << "  Archive (3D)   → Dictionary of Catalogs\n\n";
    
    // ============================================================
    // Example 1: Creating Archive and Adding Catalogs
    // ============================================================
    print_separator("1. Creating Archive and Managing Catalogs");
    
    Archive company("company_data");
    std::cout << "Created archive: " << company.name() << "\n";
    std::cout << "Initial catalogs: " << company.num_catalogs() << "\n\n";
    
    // Create and add catalogs directly using operator[]
    std::cout << "Adding catalogs:\n";
    company["employees"]["id"] = Feed(pluss::value({1.0, 2.0, 3.0}), "id");
    company["employees"]["name"] = Feed(pluss::make_note({"Alice", "Bob", "Charlie"}), "name");
    company["employees"]["salary"] = Feed(pluss::value({75000.0, 85000.0, 95000.0}), "salary");
    std::cout << "  - employees (3 rows)\n";
    
    company["departments"]["dept_id"] = Feed(pluss::value({1.0, 2.0}), "dept_id");
    company["departments"]["dept_name"] = Feed(pluss::make_note({"Engineering", "Sales"}), "dept_name");
    company["departments"]["budget"] = Feed(pluss::value({500000.0, 300000.0}), "budget");
    std::cout << "  - departments (2 rows)\n";
    
    company["projects"]["proj_id"] = Feed(pluss::value({101.0, 102.0, 103.0}), "proj_id");
    company["projects"]["proj_name"] = Feed(pluss::make_note({"WebApp", "MobileApp", "DataPipeline"}), "proj_name");
    company["projects"]["status"] = Feed(pluss::make_note({"active", "active", "completed"}), "status");
    std::cout << "  - projects (3 rows)\n\n";
    
    std::cout << "Total catalogs: " << company.num_catalogs() << "\n";
    std::cout << "Catalog names: ";
    for (const auto& key : company.keys()) {
        std::cout << key << " ";
    }
    std::cout << "\n\n";
    
    // ============================================================
    // Example 2: Accessing Catalogs
    // ============================================================
    print_separator("2. Accessing and Displaying Catalogs");
    
    std::cout << "Employees catalog:\n";
    company["employees"].display();
    
    std::cout << "\nDepartments catalog:\n";
    company["departments"].display();
    
    // Check if catalog exists
    std::cout << "\nChecking catalog existence:\n";
    std::cout << "  has('employees'): " << (company.has("employees") ? "yes" : "no") << "\n";
    std::cout << "  has('customers'): " << (company.has("customers") ? "yes" : "no") << "\n\n";
    
    // ============================================================
    // Example 3: Archive Statistics
    // ============================================================
    print_separator("3. Archive Statistics");
    
    std::cout << "Archive overview:\n";
    company.info();
    
    std::cout << "\nArchive summary:\n";
    company.summary();
    
    std::cout << "\nCatalog sizes:\n";
    auto sizes = company.catalog_sizes();
    for (const auto& [name, size] : sizes) {
        std::cout << "  " << name << ": " << size << " rows\n";
    }
    
    std::cout << "\nTotal rows across all catalogs: " << company.total_rows() << "\n\n";
    
    // ============================================================
    // Example 4: Cross-Catalog Operations
    // ============================================================
    print_separator("4. Cross-Catalog Operations");
    
    // Merge two catalogs
    std::cout << "Merging employees and departments (if they have common key):\n";
    try {
        auto merged = company.merge_catalogs("employees", "departments", "dept_id", "inner");
        std::cout << "Merged catalog:\n";
        merged.display();
    } catch (const std::exception& e) {
        std::cout << "Cannot merge: " << e.what() << "\n";
    }
    std::cout << "\n";
    
    // Union catalogs
    std::cout << "Union of catalogs:\n";
    try {
        auto combined = company.union_catalogs({"employees", "departments"});
        std::cout << "Combined catalog has " << combined.size() << " rows\n";
    } catch (const std::exception& e) {
        std::cout << "Cannot union: " << e.what() << "\n";
    }
    std::cout << "\n";
    
    // ============================================================
    // Example 5: Aggregations Across Catalogs
    // ============================================================
    print_separator("5. Aggregations Across Catalogs");
    
    std::cout << "Aggregate 'salary' field across all catalogs:\n";
    auto salary_agg = company.aggregate_field("salary", "sum");
    for (const auto& [catalog_name, value] : salary_agg) {
        std::cout << "  " << catalog_name << ": $" << value << "\n";
    }
    std::cout << "\n";
    
    std::cout << "Summary of all 'salary' aggregation:\n";
    auto salary_summary = company.aggregate_all("salary", "sum");
    salary_summary.display();
    std::cout << "\n";
    
    // ============================================================
    // Example 6: Search Operations
    // ============================================================
    print_separator("6. Search Operations");
    
    std::cout << "Find catalogs with 'name' field:\n";
    auto catalogs_with_name = company.find_catalogs_with_field("name");
    for (const auto& name : catalogs_with_name) {
        std::cout << "  - " << name << "\n";
    }
    std::cout << "\n";
    
    std::cout << "Search for 'Alice' in 'name' fields:\n";
    auto search_results = company.search_catalogs("name", "Alice");
    search_results.info();
    std::cout << "\n";
    
    // ============================================================
    // Example 7: Filter and Transform Operations
    // ============================================================
    print_separator("7. Filter and Transform Operations");
    
    std::cout << "Filter all catalogs where salary > 80000:\n";
    auto high_salary = company.filter_all("salary", [](const Feed& f) {
        return f.as_series().gt(80000.0);
    });
    std::cout << "Filtered archive has " << high_salary.num_catalogs() << " catalogs\n";
    high_salary.info();
    std::cout << "\n";
    
    std::cout << "Transform all catalogs (add a constant field):\n";
    auto transformed = company.transform_all([](const Catalog& cat) {
        Catalog result = cat;
        auto catalog_size = static_cast<double>(cat.size());
        result["row_count"] = Feed(pluss::full({static_cast<int64_t>(catalog_size)}, catalog_size), "row_count");
        return result;
    });
    std::cout << "Transformed archive:\n";
    transformed["employees"].head(3).display();
    std::cout << "\n";
    
    // ============================================================
    // Example 8: Apply Operations to All Catalogs
    // ============================================================
    print_separator("8. Apply Operations to All Catalogs");
    
    std::cout << "Adding 'archived' field to all catalogs:\n";
    company.apply_all([](Catalog& cat) {
        cat["archived"] = Feed(pluss::zeros({cat.size()}), "archived");
    });
    
    std::cout << "Updated employees catalog:\n";
    company["employees"].head(3).display();
    std::cout << "\n";
    
    // ============================================================
    // Example 9: CSV I/O Operations
    // ============================================================
    print_separator("9. CSV I/O Operations");
    
    std::cout << "Writing archive to CSV files:\n";
    try {
        company.to_csv_collection("./archive_output");
        std::cout << "  Written to ./archive_output directory\n";
    } catch (const std::exception& e) {
        std::cout << "  Error: " << e.what() << "\n";
    }
    std::cout << "\n";
    
    std::cout << "Reading CSV collection:\n";
    try {
        std::vector<std::pair<std::string, std::string>> csv_files = {
            {"../data/messy_data.csv", "messy_data"}
        };
        auto loaded_archive = Archive::read_csv_collection(csv_files);
        std::cout << "  Loaded archive with " << loaded_archive.num_catalogs() << " catalogs\n";
        loaded_archive.info();
    } catch (const std::exception& e) {
        std::cout << "  Error: " << e.what() << "\n";
    }
    std::cout << "\n";
    
    // ============================================================
    // Example 10: Archive Display
    // ============================================================
    print_separator("10. Archive Display");
    
    std::cout << "Full archive display:\n";
    company.display(5);
    std::cout << "\n";
    
    std::cout << "Archive as string:\n";
    std::cout << company.to_string() << "\n\n";
    
    // ============================================================
    // Example 11: Removing Catalogs
    // ============================================================
    print_separator("11. Removing Catalogs");
    
    std::cout << "Removing 'projects' catalog:\n";
    company.remove("projects");
    std::cout << "  Remaining catalogs: " << company.num_catalogs() << "\n";
    std::cout << "  Catalog names: ";
    for (const auto& key : company.keys()) {
        std::cout << key << " ";
    }
    std::cout << "\n\n";
    
    // ============================================================
    // Example 12: Iterating Over Catalogs
    // ============================================================
    print_separator("12. Iterating Over Catalogs");
    
    std::cout << "Iterating through all catalogs:\n";
    for (const auto& [name, catalog] : company) {
        std::cout << "  " << name << ": " << catalog.size() << " rows, " 
                  << catalog.num_fields() << " fields\n";
    }
    std::cout << "\n";
    
    // ============================================================
    // Summary
    // ============================================================
    print_separator("Summary");
    
    std::cout << "Archive (3D) provides dictionary-based catalog management:\n\n";
    std::cout << "  ✅ Catalog storage: operator[], at(), has(), remove()\n";
    std::cout << "  ✅ Archive metadata: num_catalogs(), keys(), catalog_sizes()\n";
    std::cout << "  ✅ Cross-catalog operations: merge_catalogs(), union_catalogs()\n";
    std::cout << "  ✅ Aggregations: aggregate_field(), aggregate_all()\n";
    std::cout << "  ✅ Search: search_catalogs(), find_catalogs_with_field()\n";
    std::cout << "  ✅ Batch operations: filter_all(), transform_all(), apply_all()\n";
    std::cout << "  ✅ Statistics: summary(), total_rows()\n";
    std::cout << "  ✅ I/O: read_csv_collection(), to_csv_collection()\n";
    std::cout << "  ✅ Display: info(), display(), to_string()\n\n";
    
    std::cout << "Feed-First Design Complete!\n";
    std::cout << "  Feed (1D)    → feed_ex.cpp\n";
    std::cout << "  Catalog (2D) → catalog_ex.cpp\n";
    std::cout << "  Archive (3D) → archive_ex.cpp\n\n";
    
    return 0;
}
