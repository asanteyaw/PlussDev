/**
 * @file handle_csv_example.cpp
 * @brief Comprehensive example demonstrating Handle CSV I/O and advanced features
 * 
 * This example showcases:
 * 1. Reading CSV files into Catalog
 * 2. Advanced Catalog operations (filter, sort, group_by, merge)
 * 3. Archive operations for multiple datasets
 * 4. Text processing and NLP features
 * 5. Corpus management and document analytics
 */

#include <pluss/pluss.h>
#include <iostream>
#include <vector>

using namespace pluss::handle;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

void example_1_read_csv() {
    print_separator("Example 1: Basic CSV Reading");
    
    try {
        // Read messy_data.csv from the data directory using unified load() API
        auto catalog = Catalog::load("csv", "../data/messy_data.csv", "messy_data");
        
        std::cout << "Successfully loaded CSV into Catalog\n";
        catalog.info();
        std::cout << "\nFirst 5 rows:\n";
        catalog.head(5).display();
        
    } catch (const std::exception& e) {
        std::cerr << "Error reading CSV: " << e.what() << "\n";
    }
}

void example_2_catalog_operations() {
    print_separator("Example 2: Advanced Catalog Operations");
    
    try {
        auto catalog = Catalog::load("csv", "../data/messy_data.csv", "messy_data");
        
        // Filter operation
        if (catalog.has("age")) {
            std::cout << "Filtering by age > 25:\n";
            auto age_series = catalog["age"].as_series();
            auto mask = age_series.gt(25.0);
            auto filtered = catalog.filter(mask);
            filtered.head(3).display();
        }
        
        // Sort operation
        if (catalog.has("age")) {
            std::cout << "\nSorting by age (descending):\n";
            auto sorted = catalog.sort("age", false);
            sorted.head(5).display();
        }
        
        // Group by operation
        if (catalog.has("city")) {
            std::cout << "\nGrouping by city:\n";
            auto groups = catalog.group_by("city");
            std::cout << "Number of groups: " << groups.size() << "\n";
            
            for (const auto& [city, group_cat] : groups) {
                std::cout << "  " << city << ": " << group_cat.size() << " records\n";
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error in catalog operations: " << e.what() << "\n";
    }
}

void example_3_text_processing() {
    print_separator("Example 3: Text Processing with Feed");
    
    // Create sample text data using initializer list
    Feed text_feed(pluss::make_note({
        "The quick brown fox jumps over the lazy dog",
        "Natural Language Processing is fascinating!",
        "Data science combines statistics and programming",
        "Machine learning algorithms learn from data",
        "Text analytics reveals insights from documents"
    }), "sample_texts");
    
    // Basic string operations
    std::cout << "Original texts:\n";
    text_feed.head(3).display();
    
    std::cout << "\nUppercase:\n";
    text_feed.str_upper().head(3).display();
    
    std::cout << "\nWord counts:\n";
    auto word_counts = text_feed.word_count();
    word_counts.display();
    
    std::cout << "\nUnique word counts:\n";
    auto unique_counts = text_feed.unique_word_count();
    unique_counts.display();
    
    // NLP operations
    std::cout << "\nTokenized (first text):\n";
    auto tokenized = text_feed.head(1).tokenize("word");
    tokenized.display();
    
    std::cout << "\nAfter removing stopwords (first text):\n";
    auto no_stopwords = text_feed.head(1).tokenize("word").remove_stopwords();
    no_stopwords.display();
    
    std::cout << "\nStemmed (first text):\n";
    auto stemmed = text_feed.head(1).tokenize("word").stem();
    stemmed.head(5).display();
    
    // Text statistics
    std::cout << "\nText statistics:\n";
    std::cout << "Average word length:\n";
    text_feed.avg_word_length().display();
    
    std::cout << "\nCharacter counts:\n";
    text_feed.char_count().display();
}

void example_4_text_similarity() {
    print_separator("Example 4: Text Similarity");
    
    // Create feeds using initializer lists
    Feed feed1(pluss::make_note({"machine", "learning", "algorithm"}), "doc1");
    Feed feed2(pluss::make_note({"machine", "learning", "model"}), "doc2");
    Feed feed3(pluss::make_note({"natural", "language", "processing"}), "doc3");
    
    std::cout << "Document 1: machine learning algorithm\n";
    std::cout << "Document 2: machine learning model\n";
    std::cout << "Document 3: natural language processing\n\n";
    
    std::cout << "Jaccard similarity (doc1 vs doc2): " 
              << feed1.jaccard_similarity(feed2) << "\n";
    std::cout << "Jaccard similarity (doc1 vs doc3): " 
              << feed1.jaccard_similarity(feed3) << "\n";
    std::cout << "Jaccard similarity (doc2 vs doc3): " 
              << feed2.jaccard_similarity(feed3) << "\n";
}

void example_5_archive_csv_collection() {
    print_separator("Example 5: Archive - Multiple CSV Files");
    
    try {
        // Load multiple CSV files into an Archive
        std::vector<std::pair<std::string, std::string>> files = {
            {"../data/messy_data.csv", "messy_data"},
            {"../data/bank-full.csv", "bank_data"}
        };
        
        auto archive = Archive::read_csv_collection(files);
        
        std::cout << "Loaded " << archive.num_catalogs() << " catalogs into archive\n";
        archive.info();
        
        // Summary statistics
        std::cout << "\nArchive summary:\n";
        archive.summary();
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading archive: " << e.what() << "\n";
    }
}

void example_6_corpus_management() {
    print_separator("Example 6: Corpus Management");
    
    // Create a text corpus archive
    Archive corpus("text_corpus");
    
    // Create and populate document catalogs directly in the archive
    corpus["doc1"]["content"] = Feed(pluss::make_note({
        "Artificial intelligence is transforming industries",
        "Machine learning algorithms detect patterns in data",
        "Neural networks mimic human brain structure"
    }), "content");
    corpus["doc1"]["doc_id"] = Feed(pluss::value({1.0, 2.0, 3.0}), "doc_id");
    
    corpus["doc2"]["content"] = Feed(pluss::make_note({
        "Data science combines statistics and computer science",
        "Big data analytics reveal business insights",
        "Predictive models forecast future trends"
    }), "content");
    corpus["doc2"]["doc_id"] = Feed(pluss::value({4.0, 5.0, 6.0}), "doc_id");
    
    // Corpus statistics
    std::cout << "Corpus statistics:\n";
    auto stats = corpus.corpus_statistics("content");
    stats.display();
    
    // Text preprocessing
    std::cout << "\nPreprocessed corpus:\n";
    auto preprocessed = corpus.preprocess_text_fields("content");
    if (preprocessed.has("doc1")) {
        preprocessed["doc1"].head(2).display();
    }
    
    // Cross-catalog similarity
    std::cout << "\nCross-catalog text similarity:\n";
    auto similarities = corpus.cross_catalog_similarity("content", "jaccard");
    similarities.display();
    
    // Topic filtering
    std::cout << "\nFiltering by topic 'data':\n";
    auto topic_filtered = corpus.filter_by_topic("content", "data");
    topic_filtered.info();
}

void example_7_merge_operations() {
    print_separator("Example 7: Catalog Merge Operations");
    
    // Create two catalogs to merge
    Catalog customers("customers");
    customers["id"] = Feed(pluss::value({1.0, 2.0, 3.0}), "id");
    customers["name"] = Feed(pluss::make_note({"Alice", "Bob", "Charlie"}), "name");
    customers["city"] = Feed(pluss::make_note({"NYC", "LA", "SF"}), "city");
    
    Catalog orders("orders");
    orders["id"] = Feed(pluss::value({1.0, 2.0, 1.0, 3.0}), "id");
    orders["product"] = Feed(pluss::make_note({"Laptop", "Phone", "Tablet", "Monitor"}), "product");
    orders["amount"] = Feed(pluss::value({1200.0, 800.0, 500.0, 300.0}), "amount");
    
    std::cout << "Customers catalog:\n";
    customers.display();
    
    std::cout << "\nOrders catalog:\n";
    orders.display();
    
    // Inner join
    std::cout << "\nInner join on 'id':\n";
    auto inner_merged = customers.merge(orders, "id", "inner");
    inner_merged.display();
    
    // Left join
    std::cout << "\nLeft join on 'id':\n";
    auto left_merged = customers.merge(orders, "id", "left");
    left_merged.display();
}

void example_8_write_csv() {
    print_separator("Example 8: Writing Catalog to CSV");
    
    try {
        // Create a catalog
        Catalog catalog("sample_output");
        catalog["id"] = Feed(pluss::value({1.0, 2.0, 3.0, 4.0, 5.0}), "id");
        catalog["name"] = Feed(pluss::make_note({"Alice", "Bob", "Charlie", "David", "Eve"}), "name");
        catalog["score"] = Feed(pluss::value({95.5, 87.2, 91.8, 88.9, 93.1}), "score");
        
        std::cout << "Original catalog:\n";
        catalog.display();
        
        // Write to CSV
        std::string output_file = "sample_output.csv";
        catalog.to_csv(output_file);
        std::cout << "\nCatalog written to: " << output_file << "\n";
        
        // Read it back
        auto read_back = Catalog::load("csv", output_file, "read_back");
        std::cout << "\nRead back from CSV:\n";
        read_back.display();
        
    } catch (const std::exception& e) {
        std::cerr << "Error writing CSV: " << e.what() << "\n";
    }
}

void example_9_aggregations() {
    print_separator("Example 9: Aggregations and Statistics");
    
    Catalog sales("sales");
    sales["product"] = Feed(pluss::make_note({"A", "B", "A", "C", "B", "A"}), "product");
    sales["quantity"] = Feed(pluss::value({10.0, 20.0, 15.0, 5.0, 25.0, 12.0}), "quantity");
    sales["revenue"] = Feed(pluss::value({100.0, 200.0, 150.0, 50.0, 250.0, 120.0}), "revenue");
    
    std::cout << "Sales data:\n";
    sales.display();
    
    std::cout << "\nTotal quantity sold:\n";
    auto total_qty = sales.sum("quantity");
    std::cout << "  " << total_qty.sum() << "\n";
    
    std::cout << "\nAverage revenue:\n";
    auto avg_rev = sales.mean("revenue");
    std::cout << "  " << avg_rev.mean() << "\n";
    
    std::cout << "\nMax/Min revenue:\n";
    auto max_rev = sales.max("revenue");
    auto min_rev = sales.min("revenue");
    std::cout << "  Max: " << max_rev.max() << "\n";
    std::cout << "  Min: " << min_rev.min() << "\n";
}

int main() {
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║     Handle Module CSV & Advanced Features Example       ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    
    try {
        example_1_read_csv();
        example_2_catalog_operations();
        example_3_text_processing();
        example_4_text_similarity();
        example_5_archive_csv_collection();
        example_6_corpus_management();
        example_7_merge_operations();
        example_8_write_csv();
        example_9_aggregations();
        
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "All examples completed successfully!\n";
        std::cout << std::string(60, '=') << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\nFatal error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
