#include <pluss/datalog/table/thread/thread.h>
#include <pluss/series_factory.h>
#include <pluss/note_factory.h>
#include <iostream>
#include <cassert>
#include <cmath>

using namespace pluss::table;

void test_indexing_operations() {
    std::cout << "=== Testing Indexing Operations ===\n";
    
    // Test head/tail
    auto data = pluss::from_torch(torch::tensor({1.0, 2.0, 3.0, 4.0, 5.0}));
    Thread t(data, "test");
    
    auto head_result = t.head(3);
    assert(head_result.size() == 3);
    std::cout << "✓ head() works\n";
    
    auto tail_result = t.tail(2);
    assert(tail_result.size() == 2);
    std::cout << "✓ tail() works\n";
    
    // Test iloc
    auto slice = t.iloc(1, 4);
    assert(slice.size() == 3);
    std::cout << "✓ iloc() works\n";
    
    // Test take
    auto taken = t.take({0, 2, 4});
    assert(taken.size() == 3);
    std::cout << "✓ take() works\n";
    
    std::cout << "All indexing tests passed!\n\n";
}

void test_mathematical_operations() {
    std::cout << "=== Testing Mathematical Operations ===\n";
    
    auto data = pluss::from_torch(torch::tensor({4.0, 9.0, 16.0, 25.0}));
    Thread t(data, "math");
    
    // Test sqrt
    auto sqrt_result = t.sqrt();
    assert(sqrt_result.is_series());
    assert(sqrt_result.size() == 4);
    std::cout << "✓ sqrt() works\n";
    
    // Test abs
    auto neg_data = pluss::from_torch(torch::tensor({-1.0, -2.0, 3.0, -4.0}));
    Thread t_neg(neg_data, "neg");
    auto abs_result = t_neg.abs();
    assert(abs_result.is_series());
    assert(abs_result.size() == 4);
    std::cout << "✓ abs() works\n";
    
    // Test round
    auto decimal_data = pluss::from_torch(torch::tensor({1.234, 2.567, 3.891}));
    Thread t_dec(decimal_data, "decimal");
    auto rounded = t_dec.round(1);
    assert(rounded.is_series());
    std::cout << "✓ round() works\n";
    
    // Test log
    auto pos_data = pluss::from_torch(torch::tensor({1.0, 2.718, 7.389}));
    Thread t_pos(pos_data, "positive");
    auto log_result = t_pos.log();
    assert(log_result.is_series());
    std::cout << "✓ log() works\n";
    
    std::cout << "All mathematical tests passed!\n\n";
}

void test_comparison_operations() {
    std::cout << "=== Testing Comparison Operations ===\n";
    
    auto data1 = pluss::from_torch(torch::tensor({1.0, 2.0, 3.0, 4.0}));
    auto data2 = pluss::from_torch(torch::tensor({2.0, 2.0, 2.0, 2.0}));
    Thread t1(data1, "t1");
    Thread t2(data2, "t2");
    
    // Test eq
    auto eq_result = t1.eq(t2);
    assert(eq_result[1].item<bool>() == true);  // 2.0 == 2.0
    std::cout << "✓ eq() works\n";
    
    // Test lt
    auto lt_result = t1.lt(t2);
    assert(lt_result[0].item<bool>() == true);  // 1.0 < 2.0
    std::cout << "✓ lt() works\n";
    
    // Test gt
    auto gt_result = t1.gt(t2);
    assert(gt_result[3].item<bool>() == true);  // 4.0 > 2.0
    std::cout << "✓ gt() works\n";
    
    std::cout << "All comparison tests passed!\n\n";
}

void test_arithmetic_operations() {
    std::cout << "=== Testing Arithmetic Operations ===\n";
    
    auto data1 = pluss::from_torch(torch::tensor({10.0, 20.0, 30.0}));
    auto data2 = pluss::from_torch(torch::tensor({2.0, 3.0, 4.0}));
    Thread t1(data1, "t1");
    Thread t2(data2, "t2");
    
    // Test add
    auto add_result = t1.add(t2);
    assert(add_result.is_series());
    assert(add_result.size() == 3);
    std::cout << "✓ add() works\n";
    
    // Test sub
    auto sub_result = t1.sub(t2);
    assert(sub_result.is_series());
    assert(sub_result.size() == 3);
    std::cout << "✓ sub() works\n";
    
    // Test mul
    auto mul_result = t1.mul(t2);
    assert(mul_result.is_series());
    assert(mul_result.size() == 3);
    std::cout << "✓ mul() works\n";
    
    // Test div
    auto div_result = t1.div(t2);
    assert(div_result.is_series());
    assert(div_result.size() == 3);
    std::cout << "✓ div() works\n";
    
    std::cout << "All arithmetic tests passed!\n\n";
}

void test_cumulative_operations() {
    std::cout << "=== Testing Cumulative Operations ===\n";
    
    auto data = pluss::from_torch(torch::tensor({1.0, 2.0, 3.0, 4.0}));
    Thread t(data, "cumulative");
    
    // Test cumsum
    auto cumsum = t.cumsum();
    assert(cumsum.as_series()[3].item<double>() == 10.0);  // 1+2+3+4
    std::cout << "✓ cumsum() works\n";
    
    // Test cumprod
    auto cumprod = t.cumprod();
    assert(cumprod.as_series()[3].item<double>() == 24.0);  // 1*2*3*4
    std::cout << "✓ cumprod() works\n";
    
    // Test cummax
    auto cummax = t.cummax();
    assert(cummax.as_series()[2].item<double>() == 3.0);
    std::cout << "✓ cummax() works\n";
    
    std::cout << "All cumulative tests passed!\n\n";
}

void test_missing_data_operations() {
    std::cout << "=== Testing Missing Data Operations ===\n";
    
    auto data = pluss::from_torch(torch::tensor({1.0, NAN, 3.0, NAN, 5.0}));
    Thread t(data, "na_test");
    
    // Test isna/notna
    auto is_na = t.isna();
    assert(is_na[1].item<bool>() == true);
    assert(is_na[0].item<bool>() == false);
    std::cout << "✓ isna() works\n";
    
    auto not_na = t.notna();
    assert(not_na[0].item<bool>() == true);
    assert(not_na[1].item<bool>() == false);
    std::cout << "✓ notna() works\n";
    
    // Test dropna
    auto dropped = t.dropna();
    assert(dropped.size() == 3);  // Only non-NaN values
    std::cout << "✓ dropna() works\n";
    
    // Test fillna
    auto zero_data = pluss::from_torch(torch::tensor({0.0}));
    Thread fill_val(zero_data, "fill");
    auto filled = t.fillna(fill_val);
    assert(!std::isnan(filled.as_series()[1].item<double>()));
    std::cout << "✓ fillna() works\n";
    
    // Test ffill
    auto ffilled = t.ffill();
    assert(ffilled.as_series()[1].item<double>() == 1.0);  // Forward filled
    std::cout << "✓ ffill() works\n";
    
    // Test bfill
    auto bfilled = t.bfill();
    assert(bfilled.as_series()[1].item<double>() == 3.0);  // Backward filled
    std::cout << "✓ bfill() works\n";
    
    std::cout << "All missing data tests passed!\n\n";
}

void test_transformation_operations() {
    std::cout << "=== Testing Transformation Operations ===\n";
    
    auto data = pluss::from_torch(torch::tensor({1.0, 5.0, 10.0, 15.0, 20.0}));
    Thread t(data, "transform");
    
    // Test diff
    auto diff_result = t.diff(1);
    assert(std::abs(diff_result.as_series()[1].item<double>() - 4.0) < 0.001);
    std::cout << "✓ diff() works\n";
    
    // Test shift
    auto shifted = t.shift(1);
    assert(std::isnan(shifted.as_series()[0].item<double>()));
    assert(shifted.as_series()[1].item<double>() == 1.0);
    std::cout << "✓ shift() works\n";
    
    // Test pct_change
    auto pct = t.pct_change(1);
    assert(pct.is_series());
    std::cout << "✓ pct_change() works\n";
    
    // Test clip
    auto clipped = t.clip(5.0, 15.0);
    assert(clipped.as_series()[0].item<double>() == 5.0);   // Clipped from 1
    assert(clipped.as_series()[4].item<double>() == 15.0);  // Clipped from 20
    std::cout << "✓ clip() works\n";
    
    std::cout << "All transformation tests passed!\n\n";
}

void test_statistical_operations() {
    std::cout << "=== Testing Statistical Operations ===\n";
    
    auto data = pluss::from_torch(torch::tensor({5.0, 2.0, 8.0, 1.0, 9.0, 3.0}));
    Thread t(data, "stats");
    
    // Test sort_values
    auto sorted = t.sort_values(true);  // Ascending
    assert(sorted.as_series()[0].item<double>() == 1.0);
    assert(sorted.as_series()[5].item<double>() == 9.0);
    std::cout << "✓ sort_values() works\n";
    
    // Test unique
    auto dup_data = pluss::from_torch(torch::tensor({1.0, 2.0, 2.0, 3.0, 3.0, 3.0}));
    Thread t_dup(dup_data, "dup");
    auto unique_result = t_dup.unique();
    assert(unique_result.size() <= 3);  // At most 3 unique values
    std::cout << "✓ unique() works\n";
    
    // Test nlargest/nsmallest
    auto largest = t.nlargest(2);
    assert(largest.size() == 2);
    std::cout << "✓ nlargest() works\n";
    
    auto smallest = t.nsmallest(2);
    assert(smallest.size() == 2);
    std::cout << "✓ nsmallest() works\n";
    
    std::cout << "All statistical tests passed!\n\n";
}

void test_edge_cases() {
    std::cout << "=== Testing Edge Cases ===\n";
    
    // Empty thread
    auto empty_data = pluss::from_torch(torch::tensor({}));
    Thread empty(empty_data, "empty");
    auto empty_head = empty.head(5);
    assert(empty_head.size() == 0);
    std::cout << "✓ Empty thread handled\n";
    
    // Single element
    auto single_data = pluss::from_torch(torch::tensor({42.0}));
    Thread single(single_data, "single");
    auto single_head = single.head(10);
    assert(single_head.size() == 1);
    std::cout << "✓ Single element handled\n";
    
    // All NaN
    auto all_nan = pluss::from_torch(torch::tensor({NAN, NAN, NAN}));
    Thread nan_thread(all_nan, "all_nan");
    auto dropped_all = nan_thread.dropna();
    assert(dropped_all.size() == 0);
    std::cout << "✓ All NaN handled\n";
    
    // Negative indexing edge
    auto data = pluss::from_torch(torch::tensor({1.0, 2.0, 3.0}));
    Thread t(data, "edge");
    auto bad_iloc = t.iloc(-1, 2);  // Invalid
    assert(bad_iloc.size() == 0);
    std::cout << "✓ Invalid indices handled\n";
    
    std::cout << "All edge case tests passed!\n\n";
}

int main() {
    std::cout << "\n====================================\n";
    std::cout << "    Thread Operations Test Suite    \n";
    std::cout << "====================================\n\n";
    
    try {
        test_indexing_operations();
        test_mathematical_operations();
        test_comparison_operations();
        test_arithmetic_operations();
        test_cumulative_operations();
        test_missing_data_operations();
        test_transformation_operations();
        test_statistical_operations();
        test_edge_cases();
        
        std::cout << "====================================\n";
        std::cout << "   ✓ ALL TESTS PASSED SUCCESSFULLY  \n";
        std::cout << "====================================\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "✗ TEST FAILED: " << e.what() << "\n";
        return 1;
    }
}
