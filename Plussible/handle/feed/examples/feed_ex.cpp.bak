/**
 * @file feed_ex.cpp
 * @brief Feed (1D) - Homogeneous Field Operations Examples
 * 
 * Feed is the foundation of Feed-First design:
 * - Feed: Homogeneous 1D field (Series, Note, or Timeline)
 * - Catalog: Dictionary of Feeds
 * - Archive: Collection of Catalogs
 * 
 * Feed mirrors Thread from Table module but with dictionary-style access.
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
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   Feed (1D) - Homogeneous Field Operations                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    
    // ============================================================
    // Example 1: Series Feed (Numeric Data)
    // ============================================================
    print_separator("1. Series Feed - Numeric Operations");
    
    auto scores = pluss::value({85.5, 92.0, 78.5, 88.0, 95.5});
    Feed score_feed(scores, "test_scores");
    
    std::cout << "Original scores Feed:\n";
    score_feed.display(10);
    
    std::cout << "\nStatistics:\n";
    std::cout << "  Mean:   " << score_feed.mean() << "\n";
    std::cout << "  Median: " << score_feed.median() << "\n";
    std::cout << "  Std:    " << score_feed.std() << "\n";
    std::cout << "  Min:    " << score_feed.min() << "\n";
    std::cout << "  Max:    " << score_feed.max() << "\n";
    
    // Vectorized operations
    std::cout << "\nVectorized transformations:\n";
    auto normalized = score_feed.normalize();
    std::cout << "  Normalized (0-1): ";
    normalized.display(5);
    
    auto standardized = score_feed.standardize();
    std::cout << "  Standardized (z-score): ";
    standardized.display(5);
    
    // ============================================================
    // Example 2: Note Feed (Text Data)
    // ============================================================
    print_separator("2. Note Feed - Text Operations");
    
    std::vector<Chord> names = {
        Chord("alice"),
        Chord("bob"),
        Chord("charlie"),
        Chord("diana"),
        Chord("eve")
    };
    Feed name_feed(make_note(names), "names");
    
    std::cout << "Original names:\n";
    name_feed.display(10);
    
    // String transformations
    auto upper_names = name_feed.str_upper();
    std::cout << "\nUppercase:\n";
    upper_names.display(10);
    
    auto capitalized = name_feed.str_capitalize();
    std::cout << "\nCapitalized:\n";
    capitalized.display(10);
    
    // String filtering
    auto contains_a = name_feed.str_contains("a");
    std::cout << "\nNames containing 'a':\n";
    auto filtered_names = name_feed.filter(contains_a.as_series());
    filtered_names.display(10);
    
    // ============================================================
    // Example 3: Indexing and Slicing
    // ============================================================
    print_separator("3. Indexing and Slicing");
    
    auto prices = pluss::value({19.99, 29.99, 39.99, 49.99, 59.99, 69.99, 79.99, 89.99});
    Feed price_feed(prices, "prices");
    
    std::cout << "All prices:\n";
    price_feed.display(10);
    
    std::cout << "\nFirst 3 prices (head):\n";
    price_feed.head(3).display(10);
    
    std::cout << "\nLast 3 prices (tail):\n";
    price_feed.tail(3).display(10);
    
    std::cout << "\nPrices from index 2 to 5 (slice):\n";
    price_feed.slice(2, 5).display(10);
    
    // ============================================================
    // Example 4: Filtering and Comparisons
    // ============================================================
    print_separator("4. Filtering and Comparisons");
    
    auto ages = pluss::value({25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0});
    Feed age_feed(ages, "ages");
    
    std::cout << "All ages:\n";
    age_feed.display(10);
    
    // Create boolean mask
    auto mask = age_feed.ge(Feed(pluss::value({40.0}), "threshold"));
    std::cout << "\nAges >= 40:\n";
    age_feed.filter(mask).display(10);
    
    // Between range
    auto range_mask = age_feed.between(30.0, 45.0);
    std::cout << "\nAges between 30 and 45:\n";
    age_feed.filter(range_mask).display(10);
    
    // ============================================================
    // Example 5: Sorting
    // ============================================================
    print_separator("5. Sorting Operations");
    
    auto unsorted = pluss::value({42.0, 17.0, 89.0, 23.0, 56.0, 34.0});
    Feed data_feed(unsorted, "data");
    
    std::cout << "Unsorted:\n";
    data_feed.display(10);
    
    std::cout << "\nSorted (ascending):\n";
    data_feed.sort(true).display(10);
    
    std::cout << "\nSorted (descending):\n";
    data_feed.sort(false).display(10);
    
    std::cout << "\nUnique values:\n";
    auto with_duplicates = pluss::value({1.0, 2.0, 3.0, 2.0, 4.0, 3.0, 5.0});
    Feed dup_feed(with_duplicates, "duplicates");
    dup_feed.unique().display(10);
    
    // ============================================================
    // Example 6: Mathematical Operations
    // ============================================================
    print_separator("6. Mathematical Operations");
    
    auto values = pluss::value({-4.7, 16.0, 100.0, 2.5, -9.3});
    Feed math_feed(values, "values");
    
    std::cout << "Original values:\n";
    math_feed.display(10);
    
    std::cout << "\nMathematical transformations:\n";
    std::cout << "  abs():   "; math_feed.abs().display(5);
    std::cout << "  sqrt():  "; Feed(pluss::value({16.0, 25.0, 36.0}), "squares").sqrt().display(5);
    std::cout << "  pow(2):  "; Feed(pluss::value({2.0, 3.0, 4.0}), "base").pow(2.0).display(5);
    std::cout << "  exp():   "; Feed(pluss::value({0.0, 1.0, 2.0}), "exps").exp().display(5);
    std::cout << "  log():   "; Feed(pluss::value({1.0, 2.718, 7.389}), "logs").log().display(5);
    
    // Rounding
    std::cout << "\nRounding operations:\n";
    auto decimals = pluss::value({3.14159, 2.71828, 1.41421});
    Feed decimal_feed(decimals, "decimals");
    std::cout << "  Original: "; decimal_feed.display(5);
    std::cout << "  round(2): "; decimal_feed.round(2).display(5);
    std::cout << "  floor():  "; decimal_feed.floor().display(5);
    std::cout << "  ceil():   "; decimal_feed.ceil().display(5);
    
    // ============================================================
    // Example 7: Missing Value Handling
    // ============================================================
    print_separator("7. Missing Value Handling");
    
    auto with_nan = pluss::value({1.0, std::nan(""), 3.0, std::nan(""), 5.0});
    Feed nan_feed(with_nan, "with_missing");
    
    std::cout << "Data with NaN:\n";
    nan_feed.display(10);
    
    std::cout << "\nMissing value info:\n";
    std::cout << "  has_na():   " << (nan_feed.has_na() ? "yes" : "no") << "\n";
    std::cout << "  count_na(): " << nan_feed.count_na() << "\n";
    
    std::cout << "\nFill missing with 0:\n";
    nan_feed.fill_na(0.0).display(10);
    
    std::cout << "\nDrop missing:\n";
    nan_feed.drop_na().display(10);
    
    std::cout << "\nForward fill:\n";
    nan_feed.ffill().display(10);
    
    std::cout << "\nBackward fill:\n";
    nan_feed.bfill().display(10);
    
    // ============================================================
    // Example 8: Cumulative Operations
    // ============================================================
    print_separator("8. Cumulative Operations");
    
    auto seq = pluss::value({1.0, 2.0, 3.0, 4.0, 5.0});
    Feed seq_feed(seq, "sequence");
    
    std::cout << "Original sequence:\n";
    seq_feed.display(10);
    
    std::cout << "\nCumulative operations:\n";
    std::cout << "  cumsum():  "; seq_feed.cumsum().display(10);
    std::cout << "  cumprod(): "; seq_feed.cumprod().display(10);
    std::cout << "  cummax():  "; Feed(pluss::value({3.0, 1.0, 4.0, 1.0, 5.0}), "vals").cummax().display(10);
    std::cout << "  cummin():  "; Feed(pluss::value({3.0, 1.0, 4.0, 1.0, 5.0}), "vals").cummin().display(10);
    
    // ============================================================
    // Example 9: Rolling Window Operations
    // ============================================================
    print_separator("9. Rolling Window Operations");
    
    auto time_series = pluss::value({10.0, 12.0, 15.0, 11.0, 14.0, 16.0, 13.0, 17.0});
    Feed ts_feed(time_series, "time_series");
    
    std::cout << "Time series data:\n";
    ts_feed.display(10);
    
    std::cout << "\nRolling window (size=3):\n";
    std::cout << "  rolling_mean(3): "; ts_feed.rolling_mean(3).display(10);
    std::cout << "  rolling_sum(3):  "; ts_feed.rolling_sum(3).display(10);
    std::cout << "  rolling_std(3):  "; ts_feed.rolling_std(3).display(10);
    std::cout << "  rolling_min(3):  "; ts_feed.rolling_min(3).display(10);
    std::cout << "  rolling_max(3):  "; ts_feed.rolling_max(3).display(10);
    
    // ============================================================
    // Example 10: Shift and Diff
    // ============================================================
    print_separator("10. Shift and Difference Operations");
    
    auto prices_ts = pluss::value({100.0, 102.0, 101.0, 105.0, 103.0});
    Feed price_ts(prices_ts, "stock_prices");
    
    std::cout << "Stock prices:\n";
    price_ts.display(10);
    
    std::cout << "\nShift forward by 1:\n";
    price_ts.shift(1).display(10);
    
    std::cout << "\nDaily change (diff):\n";
    price_ts.diff(1).display(10);
    
    std::cout << "\nPercent change:\n";
    price_ts.pct_change(1).display(10);
    
    // ============================================================
    // Example 11: Sampling
    // ============================================================
    print_separator("11. Sampling and Randomization");
    
    auto population = pluss::arange(1, 101);  // 1 to 100
    Feed pop_feed(population, "population");
    
    std::cout << "Population size: " << pop_feed.size() << "\n\n";
    
    std::cout << "Random sample (n=10, without replacement):\n";
    pop_feed.sample(10, false).display(10);
    
    std::cout << "\nRandom sample (n=5, with replacement):\n";
    pop_feed.sample(5, true).display(10);
    
    std::cout << "\nShuffled (first 10):\n";
    pop_feed.shuffle().head(10).display(10);
    
    // ============================================================
    // Example 12: Type Conversions
    // ============================================================
    print_separator("12. Type Conversions");
    
    // Numeric to string
    auto nums = pluss::value({42.0, 17.0, 99.0});
    Feed num_feed(nums, "numbers");
    std::cout << "Numbers as Series:\n";
    num_feed.display(5);
    
    std::cout << "\nConverted to text (Note):\n";
    num_feed.as_text().display(5);
    
    // String to numeric  
    std::vector<Chord> str_nums = {Chord("42"), Chord("17"), Chord("99")};
    Feed str_num_feed(make_note(str_nums), "string_numbers");
    std::cout << "\nString numbers:\n";
    str_num_feed.display(5);
    
    std::cout << "\nParsed to numeric:\n";
    str_num_feed.to_numeric().display(5);
    
    std::cout << "\n✅ Feed examples complete!\n\n";
    std::cout << "Key Takeaways:\n";
    std::cout << "  • Feed is homogeneous: Series, Note, or Timeline\n";
    std::cout << "  • All operations are vectorized (fast)\n";
    std::cout << "  • Feed mirrors Thread design from Table module\n";
    std::cout << "  • Dictionary-style access (not integer indices)\n\n";
    
    return 0;
}
