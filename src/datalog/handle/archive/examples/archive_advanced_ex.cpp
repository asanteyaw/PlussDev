/**
 * @file archive_advanced_ex.cpp
 * @brief Advanced Archive operations and corpus management
 * 
 * Demonstrates Archive-level operations including:
 * - Cross-catalog operations
 * - Aggregations across collections
 * - Corpus management for text analytics
 * - Statistical summaries
 */

#include <pluss/datalog/handle/archive/archive.h>
#include <pluss/series_factory.h>
#include <pluss/note_factory.h>
#include <iostream>

using namespace pluss::handle;

void example_basic_archive() {
    std::cout << "\n=== Example: Basic Archive Operations ===\n";
    
    Archive company("company_data");
    
    // Create employees catalog
    Catalog employees("employees");
    employees["id"] = Feed(pluss::value({1.0, 2.0, 3.0}), "id");
    employees["name"] = Feed(pluss::make_note({"Alice", "Bob", "Charlie"}), "name");
    employees["salary"] = Feed(pluss::value({75000.0, 85000.0, 95000.0}), "salary");
    
    // Create departments catalog
    Catalog departments("departments");
    departments["dept_id"] = Feed(pluss::value({1.0, 2.0}), "dept_id");
    departments["dept_name"] = Feed(pluss::make_note({"Engineering", "Sales"}), "dept_name");
    departments["budget"] = Feed(pluss::value({500000.0, 300000.0}), "budget");
    
    company["employees"] = employees;
    company["departments"] = departments;
    
    std::cout << "Archive created with " << company.num_catalogs() << " catalogs\n";
    company.info();
    
    std::cout << "\n--- Summary ---\n";
    company.summary();
}

void example_batch_operations() {
    std::cout << "\n=== Example: Batch Operations ===\n";
    
    Archive sales_archive("sales_by_region");
    
    // Create regional sales catalogs
    Catalog north("north");
    north["product"] = Feed(pluss::make_note({"Laptop", "Mouse", "Keyboard"}), "product");
    north["revenue"] = Feed(pluss::value({15000.0, 3000.0, 5000.0}), "revenue");
    north["units"] = Feed(pluss::value({15.0, 100.0, 50.0}), "units");
    
    Catalog south("south");
    south["product"] = Feed(pluss::make_note({"Laptop", "Monitor", "Webcam"}), "product");
    south["revenue"] = Feed(pluss::value({25000.0, 12000.0, 4000.0}), "revenue");
    south["units"] = Feed(pluss::value({25.0, 40.0, 50.0}), "units");
    
    Catalog east("east");
    east["product"] = Feed(pluss::make_note({"Mouse", "Keyboard", "Headset"}), "product");
    east["revenue"] = Feed(pluss::value({2000.0, 6000.0, 8000.0}), "revenue");
    east["units"] = Feed(pluss::value({80.0, 60.0, 40.0}), "units");
    
    sales_archive["north"] = north;
    sales_archive["south"] = south;
    sales_archive["east"] = east;
    
    std::cout << "Sales Archive:\n";
    sales_archive.info();
    
    // Aggregate revenue across all regions
    std::cout << "\n--- Revenue by Region ---\n";
    auto revenue_summary = sales_archive.aggregate_all("revenue", "sum");
    revenue_summary.display();
    
    std::cout << "\n--- Average Units by Region ---\n";
    auto units_summary = sales_archive.aggregate_all("units", "mean");
    units_summary.display();
    
    // Filter across all catalogs
    std::cout << "\n--- High Revenue Products (>10000) ---\n";
    auto high_revenue = sales_archive.filter_all("revenue", [](const Feed& f) {
        return f.gt(10000.0).as_series();
    });
    high_revenue.display();
}

void example_cross_catalog_operations() {
    std::cout << "\n=== Example: Cross-Catalog Operations ===\n";
    
    Archive store("multi_store");
    
    Catalog store_a("store_a");
    store_a["product_id"] = Feed(pluss::value({1.0, 2.0, 3.0}), "product_id");
    store_a["name"] = Feed(pluss::make_note({"Widget", "Gadget", "Tool"}), "name");
    store_a["price"] = Feed(pluss::value({10.0, 25.0, 15.0}), "price");
    
    Catalog store_b("store_b");
    store_b["product_id"] = Feed(pluss::value({1.0, 2.0, 4.0}), "product_id");
    store_b["name"] = Feed(pluss::make_note({"Widget", "Gadget", "Device"}), "name");
    store_b["price"] = Feed(pluss::value({12.0, 23.0, 30.0}), "price");
    
    store["store_a"] = store_a;
    store["store_b"] = store_b;
    
    // Merge two catalogs
    std::cout << "\n--- Merge Store A and Store B on product_id ---\n";
    auto merged = store.merge_catalogs("store_a", "store_b", "product_id", "inner");
    merged.display();
    
    // Get size statistics
    std::cout << "\n--- Catalog Sizes ---\n";
    auto sizes = store.catalog_sizes();
    for (const auto& [name, size] : sizes) {
        std::cout << name << ": " << size << " rows\n";
    }
    
    std::cout << "Total rows across all catalogs: " << store.total_rows() << "\n";
}

void example_corpus_management() {
    std::cout << "\n=== Example: Corpus Management for Text ===\n";
    
    Archive corpus("document_corpus");
    
    // Create multiple document collections
    Catalog tech_docs("tech_docs");
    tech_docs["doc_id"] = Feed(pluss::value({1.0, 2.0, 3.0}), "doc_id");
    tech_docs["content"] = Feed(pluss::make_note({
        "Machine learning algorithms improve prediction accuracy.",
        "Deep neural networks process complex data patterns.",
        "Artificial intelligence transforms business processes."
    }), "content");
    
    Catalog business_docs("business_docs");
    business_docs["doc_id"] = Feed(pluss::value({1.0, 2.0}), "doc_id");
    business_docs["content"] = Feed(pluss::make_note({
        "Strategic planning drives organizational success.",
        "Market analysis guides investment decisions."
    }), "content");
    
    Catalog research_docs("research_docs");
    research_docs["doc_id"] = Feed(pluss::value({1.0, 2.0}), "doc_id");
    research_docs["content"] = Feed(pluss::make_note({
        "Quantum computing promises computational breakthroughs.",
        "Blockchain technology ensures secure transactions."
    }), "content");
    
    corpus["tech"] = tech_docs;
    corpus["business"] = business_docs;
    corpus["research"] = research_docs;
    
    std::cout << "Document Corpus:\n";
    corpus.info();
    
    // Corpus statistics
    std::cout << "\n--- Corpus Statistics ---\n";
    auto stats = corpus.corpus_statistics("content");
    stats.display();
    
    // Preprocess all text
    std::cout << "\n--- Preprocessing All Text ---\n";
    auto preprocessed = corpus.preprocess_text_fields("content");
    std::cout << "Preprocessed " << preprocessed.num_catalogs() << " document collections\n";
    
    // Extract keywords from all collections
    std::cout << "\n--- Extracting Keywords ---\n";
    auto keywords = corpus.extract_all_keywords("content", 3);
    keywords.display(5);
    
    // Filter by topic
    std::cout << "\n--- Documents containing 'technology' or 'computing' ---\n";
    auto tech_filtered = corpus.filter_by_topic("content", "technology");
    auto comp_filtered = corpus.filter_by_topic("content", "computing");
    
    std::cout << "Collections with 'technology': " << tech_filtered.num_catalogs() << "\n";
    std::cout << "Collections with 'computing': " << comp_filtered.num_catalogs() << "\n";
}

void example_search_operations() {
    std::cout << "\n=== Example: Search Operations ===\n";
    
    Archive library("library");
    
    Catalog fiction("fiction");
    fiction["title"] = Feed(pluss::make_note({"1984", "Brave New World", "Fahrenheit 451"}), "title");
    fiction["author"] = Feed(pluss::make_note({"Orwell", "Huxley", "Bradbury"}), "author");
    fiction["year"] = Feed(pluss::value({1949.0, 1932.0, 1953.0}), "year");
    
    Catalog nonfiction("nonfiction");
    nonfiction["title"] = Feed(pluss::make_note({"Sapiens", "Educated", "Thinking Fast and Slow"}), "title");
    nonfiction["author"] = Feed(pluss::make_note({"Harari", "Westover", "Kahneman"}), "author");
    nonfiction["year"] = Feed(pluss::value({2011.0, 2018.0, 2011.0}), "year");
    
    library["fiction"] = fiction;
    library["nonfiction"] = nonfiction;
    
    // Search for books by author
    std::cout << "\n--- Search for 'Orwell' ---\n";
    auto orwell_books = library.search_catalogs("author", "Orwell");
    orwell_books.display();
    
    // Find catalogs with year field
    std::cout << "\n--- Catalogs with 'year' field ---\n";
    auto with_year = library.find_catalogs_with_field("year");
    for (const auto& name : with_year) {
        std::cout << "  - " << name << "\n";
    }
    
    // Transform all catalogs
    std::cout << "\n--- Add decade field to all catalogs ---\n";
    auto with_decade = library.transform_all([](const Catalog& cat) {
        if (!cat.has("year")) return cat;
        
        Catalog result = cat;
        auto years = cat.at("year");
        auto decades = years.apply([](const Feed& f) {
            return f.floor() / 10.0 * 10.0;
        });
        result["decade"] = decades;
        return result;
    });
    
    with_decade.display();
}

int main() {
    std::cout << "=========================================\n";
    std::cout << "  Advanced Archive Examples\n";
    std::cout << "=========================================\n";
    
    try {
        example_basic_archive();
        example_batch_operations();
        example_cross_catalog_operations();
        example_corpus_management();
        example_search_operations();
        
        std::cout << "\n=========================================\n";
        std::cout << "  All Examples Completed Successfully!\n";
        std::cout << "=========================================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
