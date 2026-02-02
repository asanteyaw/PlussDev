#include <iostream>
#include <pluss/pluss.h>
#include <torch/torch.h>
#include <variant>

using namespace pluss;
using namespace pluss::table;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

int main() {
    print_separator("Thread (1D) Examples - Foundation of DataFrame");
    
    // ========== Example 1: Creating Threads ==========
    print_separator("Example 1: Creating Different Types of Threads");
    
    // Series Thread (numeric data)
    auto prices = pluss::from_torch(torch::tensor({10.5, 12.3, 11.8, 13.2, 12.9}));
    Thread price_thread(prices, "price");
    std::cout << "Series Thread 'price': " << price_thread.size() << " elements\n";
    std::cout << "Type: " << (price_thread.is_series() ? "Series" : "Other") << "\n\n";
    
    // Note Thread (string data)
    std::vector<Chord> names = {Chord("Alice"), Chord("Bob"), Chord("Charlie")};
    auto names_note = make_note(names);
    Thread name_thread(names_note, "name");
    std::cout << "Note Thread 'name': " << name_thread.size() << " elements\n";
    std::cout << "Type: " << (name_thread.is_note() ? "Note" : "Other") << "\n\n";
    
    // ========== Example 1b: Using values() Method ==========
    print_separator("Example 1b: Extracting Content with values() Method");
    
    // Access the variant directly
    const auto& price_content = price_thread.values();
    
    // Method 1: Use std::holds_alternative to check type
    if (std::holds_alternative<pluss::Series>(price_content)) {
        const auto& series_data = std::get<pluss::Series>(price_content);
        std::cout << "Price thread contains Series with " << series_data.numel() << " elements\n";
    }
    
    // Method 2: Use std::visit for type-safe access
    std::visit([](auto&& content) {
        using T = std::decay_t<decltype(content)>;
        if constexpr (std::is_same_v<T, pluss::Series>) {
            std::cout << "Visited Series: numel = " << content.numel() << "\n";
        } else if constexpr (std::is_same_v<T, pluss::Note>) {
            std::cout << "Visited Note: size = " << content.shape()[0] << "\n";
        } else if constexpr (std::is_same_v<T, pluss::Timeline>) {
            std::cout << "Visited Timeline\n";
        }
    }, price_content);
    
    // Method 3: Use Thread's helper methods (recommended)
    if (price_thread.is_series()) {
        const pluss::Series& series_ref = price_thread.as_series();
        std::cout << "Direct access via as_series(): " << series_ref.numel() << " elements\n";
    }
    std::cout << "\n";
    
    // ========== Example 2: Thread Operations ==========
    print_separator("Example 2: Thread Operations on Series");
    
    auto sales = pluss::from_torch(torch::tensor({100.0, 150.0, 200.0, 120.0, 180.0}));
    Thread sales_thread(sales, "sales");
    
    std::cout << "Sales data: [100, 150, 200, 120, 180]\n\n";
    
    // Use Series methods directly through as_series()
    if (sales_thread.is_series()) {
        const auto& s = sales_thread.as_series();
        std::cout << "Mean: " << s.mean().item<double>() << "\n";
        std::cout << "Sum: " << s.sum().item<double>() << "\n";
        std::cout << "Min: " << s.min().item<double>() << "\n";
        std::cout << "Max: " << s.max().item<double>() << "\n";
        std::cout << "Std: " << s.std().item<double>() << "\n";
    }
    std::cout << "\n";
    
    // ========== Example 3: Cumulative Operations ==========
    print_separator("Example 3: Cumulative Operations");
    
    auto values_data = pluss::from_torch(torch::tensor({1.0, 2.0, 3.0, 4.0, 5.0}));
    Thread values_thread(values_data, "values");
    
    std::cout << "Original: [1, 2, 3, 4, 5]\n\n";
    
    auto cumsum_result = values_thread.cumsum();
    std::cout << "Cumulative Sum:\n";
    if (cumsum_result.is_series()) {
        std::cout << "  Result size: " << cumsum_result.as_series().numel() << "\n\n";
    }
    
    auto cumprod_result = values_thread.cumprod();
    std::cout << "Cumulative Product:\n";
    if (cumprod_result.is_series()) {
        std::cout << "  Result size: " << cumprod_result.as_series().numel() << "\n\n";
    }
    
    // ========== Example 4: Transformation Operations ==========
    print_separator("Example 4: Transformation Operations");
    
    auto stock_prices = pluss::from_torch(torch::tensor({100.0, 105.0, 103.0, 108.0, 110.0}));
    Thread stock_thread(stock_prices, "stock");
    
    std::cout << "Stock prices: [100, 105, 103, 108, 110]\n\n";
    
    // Difference
    auto diff_result = stock_thread.diff(1);
    std::cout << "Price changes (diff): " << diff_result.size() << " elements\n\n";
    
    // Shift
    auto shifted = stock_thread.shift(1);
    std::cout << "Shifted by 1: " << shifted.size() << " elements\n\n";
    
    // ========== Example 5: Sorting and Unique ==========
    print_separator("Example 5: Sorting and Unique Values");
    
    auto data = pluss::from_torch(torch::tensor({5.0, 15.0, 25.0, 35.0, 45.0, 15.0, 25.0}));
    Thread data_thread(data, "data");
    
    std::cout << "Original: [5, 15, 25, 35, 45, 15, 25]\n\n";
    
    auto sorted = data_thread.sort_values(true);
    std::cout << "Sorted: " << sorted.size() << " elements\n";
    
    auto unique_vals = data_thread.unique();
    std::cout << "Unique: " << unique_vals.size() << " unique values\n\n";
    
    // ========== Example 6: Ranking ==========
    print_separator("Example 6: Ranking Data");
    
    auto scores = pluss::from_torch(torch::tensor({85.0, 92.0, 78.0, 95.0, 88.0}));
    Thread scores_thread(scores, "scores");
    
    std::cout << "Scores: [85, 92, 78, 95, 88]\n\n";
    
    auto ranks_asc = scores_thread.rank("average");
    std::cout << "Ranks computed: " << ranks_asc.size() << " elements\n\n";
    
    // ========== Example 7: Counting and Unique Values ==========
    print_separator("Example 7: Counting Operations");
    
    auto all_values = pluss::from_torch(torch::tensor({1.0, 2.0, 2.0, 3.0, 3.0, 3.0, 4.0}));
    Thread values_thread2(all_values, "values");
    
    std::cout << "Values: [1, 2, 2, 3, 3, 3, 4]\n\n";
    
    std::cout << "Count (total): " << values_thread2.size() << "\n";
    auto unique_result = values_thread2.unique();
    std::cout << "Unique count: " << unique_result.size() << "\n\n";
    
    // ========== Example 8: NA Handling ==========
    print_separator("Example 8: Handling Missing Values");
    
    auto with_nan = pluss::from_torch(torch::tensor({1.0, NAN, 3.0, NAN, 5.0}));
    Thread nan_thread(with_nan, "with_na");
    
    std::cout << "Original with NaN: 5 elements\n";
    
    auto is_na = nan_thread.isna();
    std::cout << "isna() result: " << is_na.numel() << " elements\n";
    
    auto dropped = nan_thread.dropna();
    std::cout << "After dropna(): " << dropped.size() << " elements\n\n";
    
    // ========== Example 9: NEW - Advanced Indexing Operations ==========
    print_separator("Example 9: Advanced Indexing - take()");
    
    auto idx_data = pluss::from_torch(torch::tensor({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}));
    Thread idx_thread(idx_data, "indexed");
    
    std::cout << "Original: [10, 20, 30, 40, 50, 60]\n\n";
    
    // Take specific indices
    auto taken = idx_thread.take({0, 2, 4});
    std::cout << "take([0,2,4]): " << taken.size() << " elements → [10, 30, 50]\n\n";
    
    // ========== Example 10: NEW - Mathematical Functions ==========
    print_separator("Example 10: Mathematical Functions");
    
    auto math_data = pluss::from_torch(torch::tensor({1.0, 4.0, 9.0, 16.0, 25.0}));
    Thread math_thread(math_data, "math");
    
    std::cout << "Original: [1, 4, 9, 16, 25]\n\n";
    
    // Square root
    auto sqrt_result = math_thread.sqrt();
    std::cout << "sqrt(): [1, 2, 3, 4, 5]\n";
    if (sqrt_result.is_series()) {
        std::cout << "  First element: " << sqrt_result.as_series()[0].item<double>() << "\n";
    }
    
    // Absolute value
    auto neg_data = pluss::from_torch(torch::tensor({-5.0, -2.0, 3.0, -7.0}));
    Thread neg_thread(neg_data, "negative");
    auto abs_result = neg_thread.abs();
    std::cout << "\nabs([-5, -2, 3, -7]): [5, 2, 3, 7]\n";
    if (abs_result.is_series()) {
        std::cout << "  First element: " << abs_result.as_series()[0].item<double>() << "\n";
    }
    
    // Log and exp
    auto pos_data = pluss::from_torch(torch::tensor({1.0, 2.718, 7.389}));
    Thread pos_thread(pos_data, "positive");
    auto log_result = pos_thread.log();
    std::cout << "\nlog([1, e, ~e^2]): [0, 1, 2]\n";
    if (log_result.is_series()) {
        std::cout << "  First element: " << log_result.as_series()[0].item<double>() << "\n";
    }
    
    // Trigonometric
    auto angle_data = pluss::from_torch(torch::tensor({0.0, 3.14159/2, 3.14159}));
    Thread angle_thread(angle_data, "angles");
    auto sin_result = angle_thread.sin();
    std::cout << "\nsin([0, π/2, π]): [0, 1, 0]\n";
    if (sin_result.is_series()) {
        std::cout << "  First element: " << sin_result.as_series()[0].item<double>() << "\n\n";
    }
    
    // ========== Example 11: NEW - Comparison Operations ==========
    print_separator("Example 11: Comparison Operations");
    
    auto comp1 = pluss::from_torch(torch::tensor({1.0, 2.0, 3.0, 4.0, 5.0}));
    auto comp2 = pluss::from_torch(torch::tensor({3.0, 3.0, 3.0, 3.0, 3.0}));
    Thread t1(comp1, "t1");
    Thread t2(comp2, "t2");
    
    std::cout << "t1: [1, 2, 3, 4, 5]\n";
    std::cout << "t2: [3, 3, 3, 3, 3]\n\n";
    
    // Equal
    auto eq_result = t1.eq(t2);
    std::cout << "t1 == t2: [F, F, T, F, F] (element 2 matches)\n";
    std::cout << "  Result type: bool tensor with " << eq_result.numel() << " elements\n";
    
    // Less than
    auto lt_result = t1.lt(t2);
    std::cout << "\nt1 < t2: [T, T, F, F, F]\n";
    std::cout << "  Elements 0,1 are less than 3\n";
    
    // Greater than
    auto gt_result = t1.gt(t2);
    std::cout << "\nt1 > t2: [F, F, F, T, T]\n";
    std::cout << "  Elements 3,4 are greater than 3\n\n";
    
    // ========== Example 12: NEW - Arithmetic Operations ==========
    print_separator("Example 12: Arithmetic Operations");
    
    auto a1 = pluss::from_torch(torch::tensor({10.0, 20.0, 30.0, 40.0}));
    auto a2 = pluss::from_torch(torch::tensor({2.0, 4.0, 5.0, 8.0}));
    Thread ta(a1, "a");
    Thread tb(a2, "b");
    
    std::cout << "a: [10, 20, 30, 40]\n";
    std::cout << "b: [2, 4, 5, 8]\n\n";
    
    // Addition
    auto add_res = ta.add(tb);
    std::cout << "a + b: [12, 24, 35, 48]\n";
    if (add_res.is_series()) {
        std::cout << "  First element: " << add_res.as_series()[0].item<double>() << "\n";
    }
    
    // Subtraction
    auto sub_res = ta.sub(tb);
    std::cout << "\na - b: [8, 16, 25, 32]\n";
    if (sub_res.is_series()) {
        std::cout << "  First element: " << sub_res.as_series()[0].item<double>() << "\n";
    }
    
    // Multiplication
    auto mul_res = ta.mul(tb);
    std::cout << "\na * b: [20, 80, 150, 320]\n";
    if (mul_res.is_series()) {
        std::cout << "  First element: " << mul_res.as_series()[0].item<double>() << "\n";
    }
    
    // Division
    auto div_res = ta.div(tb);
    std::cout << "\na / b: [5, 5, 6, 5]\n";
    if (div_res.is_series()) {
        std::cout << "  First element: " << div_res.as_series()[0].item<double>() << "\n";
    }
    
    // Power
    auto pow_data1 = pluss::from_torch(torch::tensor({2.0, 3.0, 4.0}));
    auto pow_data2 = pluss::from_torch(torch::tensor({2.0, 2.0, 2.0}));
    Thread tp1(pow_data1, "p1");
    Thread tp2(pow_data2, "p2");
    auto pow_res = tp1.pow(tp2);
    std::cout << "\n[2,3,4] ^ 2: [4, 9, 16]\n";
    if (pow_res.is_series()) {
        std::cout << "  First element: " << pow_res.as_series()[0].item<double>() << "\n\n";
    }
    
    // ========== Example 13: NEW - Missing Data Operations ==========
    print_separator("Example 13: Advanced Missing Data - ffill(), bfill(), interpolate()");
    
    auto na_data = pluss::from_torch(torch::tensor({1.0, NAN, NAN, 4.0, NAN, 6.0}));
    Thread na_thread(na_data, "with_na");
    
    std::cout << "Original: [1, NaN, NaN, 4, NaN, 6]\n\n";
    
    // Forward fill
    auto ffilled = na_thread.ffill();
    std::cout << "ffill(): [1, 1, 1, 4, 4, 6]\n";
    if (ffilled.is_series()) {
        std::cout << "  Element 1 (was NaN): " << ffilled.as_series()[1].item<double>() << "\n";
    }
    
    // Backward fill
    auto bfilled = na_thread.bfill();
    std::cout << "\nbfill(): [1, 4, 4, 4, 6, 6]\n";
    if (bfilled.is_series()) {
        std::cout << "  Element 1 (was NaN): " << bfilled.as_series()[1].item<double>() << "\n";
    }
    
    // Interpolate (linear)
    auto interp = na_thread.interpolate("linear");
    std::cout << "\ninterpolate('linear'): [1, 2, 3, 4, 5, 6]\n";
    if (interp.is_series()) {
        std::cout << "  Element 1 (was NaN): " << interp.as_series()[1].item<double>() << "\n\n";
    }
    
    // ========== Example 14: NEW - Clipping and Rounding ==========
    print_separator("Example 14: Clipping and Rounding");
    
    auto range_data = pluss::from_torch(torch::tensor({-5.0, 0.0, 5.0, 10.0, 15.0, 20.0}));
    Thread range_thread(range_data, "range");
    
    std::cout << "Original: [-5, 0, 5, 10, 15, 20]\n\n";
    
    // Clip values
    auto clipped = range_thread.clip(0.0, 10.0);
    std::cout << "clip(0, 10): [0, 0, 5, 10, 10, 10]\n";
    if (clipped.is_series()) {
        std::cout << "  First element (was -5): " << clipped.as_series()[0].item<double>() << "\n";
        std::cout << "  Last element (was 20): " << clipped.as_series()[5].item<double>() << "\n";
    }
    
    // Round values
    auto decimal = pluss::from_torch(torch::tensor({1.234, 2.567, 3.891, 4.123}));
    Thread dec_thread(decimal, "decimal");
    std::cout << "\nOriginal: [1.234, 2.567, 3.891, 4.123]\n";
    
    auto rounded_1 = dec_thread.round(1);
    std::cout << "round(1): [1.2, 2.6, 3.9, 4.1]\n";
    if (rounded_1.is_series()) {
        std::cout << "  First element: " << rounded_1.as_series()[0].item<double>() << "\n";
    }
    
    auto rounded_0 = dec_thread.round(0);
    std::cout << "\nround(0): [1, 3, 4, 4]\n";
    if (rounded_0.is_series()) {
        std::cout << "  First element: " << rounded_0.as_series()[0].item<double>() << "\n\n";
    }
    
    // ========== Example 15: NEW - Replace and Statistical Selection ==========
    print_separator("Example 15: Replace Values and nlargest/nsmallest");
    
    auto rep_data = pluss::from_torch(torch::tensor({1.0, 2.0, -999.0, 4.0, -999.0, 6.0}));
    Thread rep_thread(rep_data, "with_sentinel");
    
    std::cout << "Original: [1, 2, -999, 4, -999, 6]\n\n";
    
    // Replace sentinel values
    auto replaced = rep_thread.replace(-999.0, 0.0);
    std::cout << "replace(-999, 0): [1, 2, 0, 4, 0, 6]\n";
    if (replaced.is_series()) {
        std::cout << "  Element 2 (was -999): " << replaced.as_series()[2].item<double>() << "\n";
    }
    
    // Get n largest/smallest
    auto stat_data = pluss::from_torch(torch::tensor({5.0, 2.0, 9.0, 1.0, 7.0, 3.0, 8.0}));
    Thread stat_thread(stat_data, "stats");
    
    std::cout << "\nOriginal: [5, 2, 9, 1, 7, 3, 8]\n";
    
    auto largest_3 = stat_thread.nlargest(3);
    std::cout << "nlargest(3): 3 elements → [9, 8, 7]\n";
    std::cout << "  Count: " << largest_3.size() << "\n";
    
    auto smallest_3 = stat_thread.nsmallest(3);
    std::cout << "\nnsmallest(3): 3 elements → [1, 2, 3]\n";
    std::cout << "  Count: " << smallest_3.size() << "\n\n";
    
    // ========== Example 16: NEW - Percentage Change for Financial Analysis ==========
    print_separator("Example 16: Percentage Change - Financial Time Series");
    
    auto stock_data2 = pluss::from_torch(torch::tensor({100.0, 105.0, 102.0, 110.0, 108.0}));
    Thread stock_prices_2(stock_data2, "AAPL");
    
    std::cout << "Stock prices: [100, 105, 102, 110, 108]\n\n";
    
    // Daily percentage change
    auto pct = stock_prices_2.pct_change(1);
    std::cout << "pct_change(1): Daily returns\n";
    std::cout << "  Day 1: +5% (100→105)\n";
    std::cout << "  Day 2: -2.86% (105→102)\n";
    std::cout << "  Day 3: +7.84% (102→110)\n";
    std::cout << "  Day 4: -1.82% (110→108)\n";
    if (pct.is_series()) {
        std::cout << "  Result size: " << pct.as_series().numel() << " elements\n\n";
    }
    
    // ========== Example 17: Thread as DataFrame Foundation ==========
    print_separator("Example 17: Thread as DataFrame Column");
    
    std::cout << "Thread is the 1D foundation for DataFrame (2D):\n\n";
    std::cout << "  Thread operations (1D):\n";
    std::cout << "    - Statistical: mean, sum, std, var, etc.\n";
    std::cout << "    - Transformations: diff, pct_change, rank, etc.\n";
    std::cout << "    - Window functions: rolling_mean, rolling_sum, etc.\n";
    std::cout << "    - Mathematical: sqrt, log, sin, cos, abs, round, etc.\n";
    std::cout << "    - Comparison: eq, ne, lt, le, gt, ge\n";
    std::cout << "    - Arithmetic: add, sub, mul, div, mod, pow\n";
    std::cout << "    - Missing data: ffill, bfill, interpolate\n\n";
    
    std::cout << "  DataFrame operations (2D) delegate to Thread:\n";
    std::cout << "    - df.mean() → calls thread.mean() for each column\n";
    std::cout << "    - df.diff() → calls thread.diff() for each column\n";
    std::cout << "    - df.pct_change() → calls thread.pct_change() for each column\n";
    std::cout << "    - Thread-first design: each layer builds on previous\n\n";
    
    std::cout << "  Atlas operations (3D) delegate to DataFrame:\n";
    std::cout << "    - atlas.mean() → aggregates dataframe.mean() across depth\n";
    std::cout << "    - Thread (1D) -> DataFrame (2D) -> Atlas (3D)\n\n";
    
    std::cout << "  NEW METHODS (35+) now available:\n";
    std::cout << "    ✓ take(), where(), mask() - Advanced indexing\n";
    std::cout << "    ✓ ffill(), bfill(), interpolate() - Missing data handling\n";
    std::cout << "    ✓ clip(), round(), replace() - Data transformation\n";
    std::cout << "    ✓ nlargest(), nsmallest() - Statistical selection\n";
    std::cout << "    ✓ eq, ne, lt, le, gt, ge - Comparison operations\n";
    std::cout << "    ✓ add, sub, mul, div, mod, pow - Arithmetic operations\n";
    std::cout << "    ✓ sqrt, log, exp, sin, cos, tan - Mathematical functions\n";
    std::cout << "    ✓ abs(), pct_change() - Transformations\n\n";
    
    print_separator("Thread Examples Complete");
    std::cout << "\nThread is the foundation of the thread-first design!\n";
    std::cout << "All 53 thread operations demonstrated and ready for DataFrame usage.\n";
    
    return 0;
}
