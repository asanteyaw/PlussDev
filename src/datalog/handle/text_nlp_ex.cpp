/**
 * @file text_nlp_ex.cpp
 * @brief Comprehensive examples of text processing and NLP operations
 * 
 * Demonstrates Handle module's text specialization:
 * - Text preprocessing and cleaning
 * - Tokenization and n-grams
 * - Text statistics and features
 * - Text similarity and distance
 * - Document-level operations
 * - Corpus management
 */

#include <pluss/datalog/handle/feed/feed.h>
#include <pluss/datalog/handle/catalog/catalog.h>
#include <pluss/datalog/handle/archive/archive.h>
#include <pluss/note_factory.h>
#include <pluss/series_factory.h>
#include <iostream>

using namespace pluss::handle;

void example_text_preprocessing() {
    std::cout << "\n=== Example: Text Preprocessing ===\n";
    
    auto texts = pluss::make_note({
        "Hello, World! How are YOU today?",
        "Machine Learning is AMAZING!!!",
        "Python, Java, and C++ are programming languages."
    });
    
    Feed feed(texts, "raw_text");
    
    std::cout << "Original Text:\n";
    auto original = feed.as_note().vector();
    for (size_t i = 0; i < original.size(); ++i) {
        std::cout << i << ": " << original[i].str() << "\n";
    }
    
    // Convert to lowercase
    std::cout << "\n--- Lowercase ---\n";
    auto lower = feed.str_lower().as_note().vector();
    for (size_t i = 0; i < lower.size(); ++i) {
        std::cout << i << ": " << lower[i].str() << "\n";
    }
    
    // Remove punctuation
    std::cout << "\n--- Remove Punctuation ---\n";
    auto no_punct = feed.remove_punctuation().as_note().vector();
    for (size_t i = 0; i < no_punct.size(); ++i) {
        std::cout << i << ": " << no_punct[i].str() << "\n";
    }
    
    // Combined preprocessing
    std::cout << "\n--- Fully Preprocessed (lowercase + no punct + trim) ---\n";
    auto processed = feed.str_lower().remove_punctuation().str_trim().as_note().vector();
    for (size_t i = 0; i < processed.size(); ++i) {
        std::cout << i << ": " << processed[i].str() << "\n";
    }
}

void example_tokenization() {
    std::cout << "\n=== Example: Tokenization ===\n";
    
    auto texts = pluss::make_note({
        "Natural language processing is powerful",
        "Machine learning models require data"
    });
    
    Feed feed(texts, "sentences");
    
    // Whitespace tokenization
    std::cout << "--- Whitespace Tokenization ---\n";
    auto tokens_ws = feed.tokenize("whitespace").as_note().vector();
    std::cout << "Tokens: ";
    for (const auto& token : tokens_ws) {
        std::cout << token.str() << " | ";
    }
    std::cout << "\n";
    
    // Word tokenization  
    std::cout << "\n--- Word Tokenization ---\n";
    auto tokens_word = feed.tokenize("word").as_note().vector();
    std::cout << "Tokens: ";
    for (const auto& token : tokens_word) {
        std::cout << token.str() << " | ";
    }
    std::cout << "\n";
}

void example_stopword_removal() {
    std::cout << "\n=== Example: Stopword Removal ===\n";
    
    auto words = pluss::make_note({
        "machine", "learning", "is", "the", "future", "of", "artificial", "intelligence"
    });
    
    Feed feed(words, "words");
    
    std::cout << "Original Words:\n";
    auto original = feed.as_note().vector();
    for (const auto& word : original) {
        std::cout << word.str() << " ";
    }
    std::cout << "\n";
    
    std::cout << "\n--- After Removing Stopwords ---\n";
    auto filtered = feed.remove_stopwords().as_note().vector();
    for (const auto& word : filtered) {
        std::cout << word.str() << " ";
    }
    std::cout << "\n";
}

void example_stemming() {
    std::cout << "\n=== Example: Stemming ===\n";
    
    auto words = pluss::make_note({
        "running", "runs", "ran", "easily", "fairly", "programming", "programs"
    });
    
    Feed feed(words, "words");
    
    std::cout << "Original -> Stemmed:\n";
    auto original = feed.as_note().vector();
    auto stemmed = feed.stem().as_note().vector();
    
    for (size_t i = 0; i < original.size(); ++i) {
        std::cout << original[i].str() << " -> " << stemmed[i].str() << "\n";
    }
}

void example_ngrams() {
    std::cout << "\n=== Example: N-grams ===\n";
    
    auto words = pluss::make_note({
        "the", "quick", "brown", "fox", "jumps", "over"
    });
    
    Feed feed(words, "words");
    
    // Bigrams (2-grams)
    std::cout << "--- Bigrams ---\n";
    auto bigrams = feed.ngrams(2).as_note().vector();
    for (const auto& bigram : bigrams) {
        std::cout << bigram.str() << "\n";
    }
    
    // Trigrams (3-grams)
    std::cout << "\n--- Trigrams ---\n";
    auto trigrams = feed.ngrams(3).as_note().vector();
    for (const auto& trigram : trigrams) {
        std::cout << trigram.str() << "\n";
    }
}

void example_text_statistics() {
    std::cout << "\n=== Example: Text Statistics ===\n";
    
    auto docs = pluss::make_note({
        "This is a short sentence.",
        "This is a longer sentence with more words in it.",
        "Machine learning! Is it really the future? Yes, definitely!"
    });
    
    Feed feed(docs, "documents");
    
    std::cout << "Documents:\n";
    auto original = feed.as_note().vector();
    for (size_t i = 0; i < original.size(); ++i) {
        std::cout << i << ": " << original[i].str() << "\n";
    }
    
    // Character count
    std::cout << "\n--- Character Counts ---\n";
    auto char_counts = feed.char_count();
    auto chars = char_counts.as_series();
    for (int64_t i = 0; i < chars.numel(); ++i) {
        std::cout << "Doc " << i << ": " << chars[i].item<double>() << " characters\n";
    }
    
    // Word count
    std::cout << "\n--- Word Counts ---\n";
    auto word_counts = feed.word_count();
    auto words = word_counts.as_series();
    for (int64_t i = 0; i < words.numel(); ++i) {
        std::cout << "Doc " << i << ": " << words[i].item<double>() << " words\n";
    }
    
    // Sentence count
    std::cout << "\n--- Sentence Counts ---\n";
    auto sent_counts = feed.sentence_count();
    auto sents = sent_counts.as_series();
    for (int64_t i = 0; i < sents.numel(); ++i) {
        std::cout << "Doc " << i << ": " << sents[i].item<double>() << " sentences\n";
    }
    
    // Average word length
    std::cout << "\n--- Average Word Lengths ---\n";
    auto avg_lengths = feed.avg_word_length();
    auto avgs = avg_lengths.as_series();
    for (int64_t i = 0; i < avgs.numel(); ++i) {
        std::cout << "Doc " << i << ": " << avgs[i].item<double>() << " chars/word\n";
    }
}

void example_text_similarity() {
    std::cout << "\n=== Example: Text Similarity ===\n";
    
    auto doc1_words = pluss::make_note({"machine", "learning", "deep", "neural", "networks"});
    auto doc2_words = pluss::make_note({"machine", "learning", "artificial", "intelligence"});
    auto doc3_words = pluss::make_note({"quantum", "computing", "physics", "algorithms"});
    
    Feed feed1(doc1_words, "doc1");
    Feed feed2(doc2_words, "doc2");
    Feed feed3(doc3_words, "doc3");
    
    std::cout << "Doc1 words: machine learning deep neural networks\n";
    std::cout << "Doc2 words: machine learning artificial intelligence\n";
    std::cout << "Doc3 words: quantum computing physics algorithms\n\n";
    
    // Jaccard similarity
    std::cout << "--- Jaccard Similarity ---\n";
    double sim_1_2 = feed1.jaccard_similarity(feed2);
    double sim_1_3 = feed1.jaccard_similarity(feed3);
    double sim_2_3 = feed2.jaccard_similarity(feed3);
    
    std::cout << "Doc1 vs Doc2: " << sim_1_2 << "\n";
    std::cout << "Doc1 vs Doc3: " << sim_1_3 << "\n";
    std::cout << "Doc2 vs Doc3: " << sim_2_3 << "\n";
}

void example_document_collection() {
    std::cout << "\n=== Example: Document Collection Processing ===\n";
    
    Catalog docs("documents");
    
    docs["doc_id"] = Feed(pluss::value({1.0, 2.0, 3.0, 4.0}), "doc_id");
    docs["content"] = Feed(pluss::make_note({
        "Machine learning algorithms are transforming AI",
        "Natural language processing enables text understanding",
        "Deep learning models require massive datasets",
        "Artificial intelligence impacts every industry"
    }), "content");
    docs["category"] = Feed(pluss::make_note({"AI", "NLP", "ML", "AI"}), "category");
    
    std::cout << "Document Collection:\n";
    docs.display();
    
    // Add text features
    std::cout << "\n--- With Text Features ---\n";
    auto with_features = docs.add_text_features("content");
    with_features.display();
    
    // Preprocess text
    std::cout << "\n--- Preprocessed Text ---\n";
    auto preprocessed = docs.text_preprocess("content");
    preprocessed.select({"doc_id", "content"}).display();
    
    // Filter by text pattern
    std::cout << "\n--- Documents mentioning 'learning' ---\n";
    auto learning_docs = docs.filter_by_text("content", "learning");
    learning_docs.display();
}

void example_corpus_analytics() {
    std::cout << "\n=== Example: Corpus Analytics ===\n";
    
    Archive corpus("research_corpus");
    
    // AI Research
    Catalog ai_papers("ai_papers");
    ai_papers["paper_id"] = Feed(pluss::value({1.0, 2.0, 3.0}), "paper_id");
    ai_papers["abstract"] = Feed(pluss::make_note({
        "Deep learning architectures for image recognition",
        "Reinforcement learning in game playing systems",
        "Transfer learning improves model generalization"
    }), "abstract");
    
    // NLP Research
    Catalog nlp_papers("nlp_papers");
    nlp_papers["paper_id"] = Feed(pluss::value({1.0, 2.0}), "paper_id");
    nlp_papers["abstract"] = Feed(pluss::make_note({
        "Transformer models revolutionize language understanding",
        "Attention mechanisms enhance sequence processing"
    }), "abstract");
    
    // Robotics Research
    Catalog robotics_papers("robotics_papers");
    robotics_papers["paper_id"] = Feed(pluss::value({1.0, 2.0}), "paper_id");
    robotics_papers["abstract"] = Feed(pluss::make_note({
        "Autonomous navigation using visual sensors",
        "Robot manipulation with imitation learning"
    }), "abstract");
    
    corpus["ai"] = ai_papers;
    corpus["nlp"] = nlp_papers;
    corpus["robotics"] = robotics_papers;
    
    std::cout << "Research Corpus:\n";
    corpus.info();
    
    // Corpus statistics
    std::cout << "\n--- Corpus Statistics ---\n";
    auto stats = corpus.corpus_statistics("abstract");
    stats.display();
    
    // Search across corpus
    std::cout << "\n--- Papers mentioning 'learning' ---\n";
    auto learning_papers = corpus.filter_by_topic("abstract", "learning");
    learning_papers.display();
    
    // Extract keywords from all collections
    std::cout << "\n--- Extract Keywords (top 3) ---\n";
    auto keywords = corpus.extract_all_keywords("abstract", 3);
    std::cout << "Extracted keywords from " << keywords.num_catalogs() << " collections\n";
    
    // Aggregate text statistics
    std::cout << "\n--- Average Abstract Length by Collection ---\n";
    auto length_summary = corpus.aggregate_field("abstract", "count");
    for (const auto& [collection, count] : length_summary) {
        std::cout << collection << ": " << count << " papers\n";
    }
}

void example_text_pipeline() {
    std::cout << "\n=== Example: Complete Text Processing Pipeline ===\n";
    
    Catalog reviews("product_reviews");
    
    reviews["review_id"] = Feed(pluss::value({1.0, 2.0, 3.0}), "review_id");
    reviews["text"] = Feed(pluss::make_note({
        "This product is AMAZING!!! I love it so much. Highly recommended!!!",
        "Not bad, but could be better. The quality is okay for the price.",
        "Terrible experience. Do NOT buy this product. Total waste of money."
    }), "text");
    reviews["rating"] = Feed(pluss::value({5.0, 3.0, 1.0}), "rating");
    
    std::cout << "Original Reviews:\n";
    reviews.display();
    
    // Step 1: Add text features
    std::cout << "\n--- Step 1: Add Text Features ---\n";
    auto step1 = reviews.add_text_features("text");
    step1.display();
    
    // Step 2: Preprocess text
    std::cout << "\n--- Step 2: Preprocess Text ---\n";
    auto step2 = step1.text_preprocess("text");
    step2.select({"review_id", "text", "rating"}).display();
    
    // Step 3: Extract keywords
    std::cout << "\n--- Step 3: Extract Keywords ---\n";
    auto step3 = step2.extract_keywords("text", 3);
    step3.select({"review_id", "rating", "text_keywords"}).display();
    
    // Step 4: Compute text statistics
    std::cout << "\n--- Step 4: Aggregate Statistics ---\n";
    auto total_words = reviews.aggregate_text_stats("text", "word_count");
    auto avg_length = reviews.aggregate_text_stats("text", "avg_length");
    
    std::cout << "Total Words (all reviews): " << total_words.sum() << "\n";
    std::cout << "Average Text Length: " << avg_length.mean() << " characters\n";
}

int main() {
    std::cout << "=========================================\n";
    std::cout << "  Text & NLP Processing Examples\n";
    std::cout << "=========================================\n";
    
    try {
        example_text_preprocessing();
        example_tokenization();
        example_stopword_removal();
        example_stemming();
        example_ngrams();
        example_text_statistics();
        example_text_similarity();
        example_document_collection();
        example_corpus_analytics();
        example_text_pipeline();
        
        std::cout << "\n=========================================\n";
        std::cout << "  All NLP Examples Completed!\n";
        std::cout << "  Handle Module: Rich & Text-Optimized\n";
        std::cout << "=========================================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
