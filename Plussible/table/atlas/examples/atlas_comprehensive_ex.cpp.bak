#include <pluss/pluss.h>
#include <iostream>

using namespace pluss::table;

void print_header(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(70, '=') << "\n";
}

int main() {
    try {
        std::cout << "\n╔══════════════════════════════════════════════════════╗\n";
        std::cout << "║       ATLAS 3D - Comprehensive Method Examples       ║\n";
        std::cout << "║     Using messy_data.csv and bank-full.csv Data      ║\n";
        std::cout << "╚══════════════════════════════════════════════════════╝\n";

        // Load DataFrames from CSV
        print_header("Loading Data");
        auto df1 = DataFrame::load("csv", "../data/messy_data.csv");
        auto df2 = DataFrame::load("csv", "../data/bank-full.csv");
        
        std::cout << "messy_data.csv: " << df1->shape().first << " rows × " << df1->shape().second << " cols\n";
        std::cout << "bank-full.csv: " << df2->shape().first << " rows × " << df2->shape().second << " cols\n";
        
        // Create subsets using iloc to avoid ambiguity
        std::shared_ptr<DataFrame> df1_small = df1->iloc(0, 10);
        std::shared_ptr<DataFrame> df2_small = df2->iloc(0, 10);
        int64_t df1_len = std::get<0>(df1->shape());
        std::shared_ptr<DataFrame> df3_small = df1->iloc(df1_len - 10, df1_len);
        
        // CONSTRUCTION
        print_header("1. Atlas Construction");
        std::vector<std::shared_ptr<DataFrame>> frames = {df1_small, df2_small, df3_small};
        auto atlas = std::make_shared<Atlas>(frames);
        std::cout << "Basic atlas: " << atlas->ndepth() << " depths, " << atlas->nrows() << " rows, " << atlas->ncols() << " cols\n";
        
        std::vector<std::string> names = {"morning", "afternoon", "evening"};
        auto named_atlas = std::make_shared<Atlas>(frames, names);
        std::cout << "Named atlas depths: ";
        for (const auto& n : named_atlas->depth_names()) std::cout << n << " ";
        std::cout << "\n";
        
        // ACCESS
        print_header("2. Atlas Access & Indexing");
        auto frame0 = named_atlas->at(0);
        std::cout << "at(0) → " << frame0->shape().first << "×" << frame0->shape().second << "\n";
        
        auto frame_afternoon = named_atlas->at("afternoon");
        std::cout << "at('afternoon') → " << frame_afternoon->shape().first << "×" << frame_afternoon->shape().second << "\n";
        
        auto frame_bracket = (*named_atlas)[1];
        std::cout << "operator[1] → " << frame_bracket->shape().first << "×" << frame_bracket->shape().second << "\n";
        
        // METADATA
        print_header("3. Metadata Methods");
        std::cout << "ndepth() = " << atlas->ndepth() << "\n";
        std::cout << "nrows() = " << atlas->nrows() << "\n";
        std::cout << "ncols() = " << atlas->ncols() << "\n";
        std::cout << "size() = " << atlas->size() << "\n";
        std::cout << "empty() = " << (atlas->empty() ? "true" : "false") << "\n";
        
        auto shape = atlas->shape();
        std::cout << "shape() = (" << std::get<0>(shape) << ", " << std::get<1>(shape) << ", " << std::get<2>(shape) << ")\n";
        
        auto cols = atlas->column_names();
        std::cout << "column_names() count = " << cols.size() << "\n";
        
        // SLICING
        print_header("4. Slicing Operations");
        std::vector<std::shared_ptr<DataFrame>> five_frames;
        for (int i = 0; i < 5; ++i) five_frames.push_back(df1->iloc(0, 8));
        auto large_atlas = std::make_shared<Atlas>(five_frames);
        
        std::cout << "Original: " << large_atlas->ndepth() << " depths\n";
        std::cout << "head(3): " << large_atlas->head(3)->ndepth() << " depths\n";
        std::cout << "tail(2): " << large_atlas->tail(2)->ndepth() << " depths\n";
        std::cout << "iloc(0,3): " << large_atlas->iloc(0,3)->nrows() << " rows (depth slice)\n";
        std::cout << "select_depths([0,2]): " << large_atlas->select_depths(std::vector<int64_t>{0,2})->ndepth() << " depths\n";
        std::cout << "take([1,3]): " << large_atlas->take(std::vector<int64_t>{1,3})->ndepth() << " depths\n";
        std::cout << "truncate(2,5): " << large_atlas->truncate(2,5)->nrows() << " rows\n";
        
        // SORTING
        print_header("5. Sorting & Reordering");
        std::vector<std::string> unsorted = {"zebra", "apple", "mango"};
        std::vector<std::shared_ptr<DataFrame>> sort_frames;
        for (int i = 0; i < 3; ++i) sort_frames.push_back(df1->iloc(0, 5));
        auto unsorted_atlas = std::make_shared<Atlas>(sort_frames, unsorted);
        
        std::cout << "Original: ";
        for (const auto& n : unsorted_atlas->depth_names()) std::cout << n << " ";
        std::cout << "\n";
        
        std::cout << "sort_index(true): ";
        for (const auto& n : unsorted_atlas->sort_index(true)->depth_names()) std::cout << n << " ";
        std::cout << "\n";
        
        std::vector<std::string> new_order = {"mango", "zebra", "apple"};
        std::cout << "reindex: ";
        for (const auto& n : unsorted_atlas->reindex(new_order)->depth_names()) std::cout << n << " ";
        std::cout << "\n";
        
        // FILTERING
        print_header("6. Filtering (drop_depths)");
        std::cout << "Original: 3 depths\n";
        std::cout << "drop_depths([0]): " << unsorted_atlas->drop_depths(std::vector<int64_t>{0})->ndepth() << " depths\n";
        std::cout << "drop_depths(['zebra']): " << unsorted_atlas->drop_depths(std::vector<std::string>{"zebra"})->ndepth() << " depths\n";
        
        // TRANSFORMATIONS
        print_header("7. Mathematical Transformations");
        auto s1 = pluss::from_torch(torch::series({-10.0, 5.0, 3.14}));
        auto df_math = std::make_shared<DataFrame>(std::vector<Thread>{Thread(s1, "data")});
        auto math_atlas = std::make_shared<Atlas>(std::vector<std::shared_ptr<DataFrame>>{df_math, df_math});
        
        std::cout << "abs() - Applied\n";
        (void)math_atlas->abs();
        
        std::cout << "sqrt(), log(), log10(), exp() - Applied\n";
        auto positive = pluss::from_torch(torch::series({4.0, 9.0, 16.0}));
        auto df_pos = std::make_shared<DataFrame>(std::vector<Thread>{Thread(positive, "vals")});
        auto pos_atlas = std::make_shared<Atlas>(std::vector<std::shared_ptr<DataFrame>>{df_pos});
        (void)pos_atlas->sqrt();
        (void)pos_atlas->log();
        (void)pos_atlas->log10();
        
        std::cout << "sin(), cos(), tan() - Applied\n";
        (void)math_atlas->sin();
        (void)math_atlas->cos();
        (void)math_atlas->tan();
        
        std::cout << "clip(-5.0, 5.0) - Applied\n";
        (void)math_atlas->clip(-5.0, 5.0);
        
        std::cout << "round(2) - Applied\n";
        (void)math_atlas->round(2);
        
        // CUMULATIVE
        print_header("8. Cumulative Operations");
        auto s_cum = pluss::from_torch(torch::series({1.0, 2.0, 3.0, 4.0}));
        auto df_cum = std::make_shared<DataFrame>(std::vector<Thread>{Thread(s_cum, "data")});
        auto cum_atlas = std::make_shared<Atlas>(std::vector<std::shared_ptr<DataFrame>>{df_cum, df_cum});
        
        std::cout << "cumsum() - Within DataFrames\n";
        (void)cum_atlas->cumsum();
        
        std::cout << "cumprod() - Within DataFrames\n";
        (void)cum_atlas->cumprod();
        
        std::cout << "cummax(), cummin() - Within DataFrames\n";
        (void)cum_atlas->cummax();
        (void)cum_atlas->cummin();
        
        std::cout << "cumsum_depth() - Across depths → DataFrame\n";
        auto df_cumsum = cum_atlas->cumsum_depth();
        auto cumsum_shape = df_cumsum->shape();
        std::cout << "  Result: " << std::get<0>(cumsum_shape) << "×" << std::get<1>(cumsum_shape) << "\n";
        
        std::cout << "cumprod_depth(), cummax_depth(), cummin_depth() - Across depths\n";
        (void)cum_atlas->cumprod_depth();
        (void)cum_atlas->cummax_depth();
        (void)cum_atlas->cummin_depth();
        
        // AGGREGATIONS
        print_header("9. Aggregation Operations");
        auto s_agg = pluss::from_torch(torch::series({10.0, 20.0, 30.0}));
        auto df_agg = std::make_shared<DataFrame>(std::vector<Thread>{Thread(s_agg, "values")});
        auto agg_atlas = std::make_shared<Atlas>(
            std::vector<std::shared_ptr<DataFrame>>{df_agg, df_agg, df_agg},
            std::vector<std::string>{"A", "B", "C"}
        );
        
        std::cout << "Within-DataFrame aggregations (return Atlas):\n";
        std::cout << "  sum(), mean(), median(), min(), max()\n";
        (void)agg_atlas->sum();
        (void)agg_atlas->mean();
        (void)agg_atlas->median();
        (void)agg_atlas->min();
        (void)agg_atlas->max();
        
        std::cout << "  std(), var(), prod(), count()\n";
        (void)agg_atlas->std();
        (void)agg_atlas->var();
        (void)agg_atlas->prod();
        (void)agg_atlas->count();
        
        std::cout << "\nAcross-depth aggregations (return DataFrame):\n";
        std::cout << "  sum_depth(), mean_depth(), median_depth()\n";
        auto df_sum = agg_atlas->sum_depth();
        auto sum_shape = df_sum->shape();
        std::cout << "    sum_depth result: " << std::get<0>(sum_shape) << "×" << std::get<1>(sum_shape) << "\n";
        (void)agg_atlas->mean_depth();
        (void)agg_atlas->median_depth();
        
        std::cout << "  min_depth(), max_depth(), std_depth(), var_depth()\n";
        (void)agg_atlas->min_depth();
        (void)agg_atlas->max_depth();
        (void)agg_atlas->std_depth();
        (void)agg_atlas->var_depth();
        
        // ARITHMETIC
        print_header("10. Arithmetic Operations");
        auto s1_ar = pluss::from_torch(torch::series({10.0, 20.0}));
        auto s2_ar = pluss::from_torch(torch::series({5.0, 10.0}));
        auto df_a = std::make_shared<DataFrame>(std::vector<Thread>{Thread(s1_ar, "A")});
        auto df_b = std::make_shared<DataFrame>(std::vector<Thread>{Thread(s2_ar, "A")});
        auto atlas_a = std::make_shared<Atlas>(std::vector<std::shared_ptr<DataFrame>>{df_a, df_a});
        auto atlas_b = std::make_shared<Atlas>(std::vector<std::shared_ptr<DataFrame>>{df_b, df_b});
        
        std::cout << "add(), sub(), mul(), div(), mod(), pow()\n";
        (void)atlas_a->add(atlas_b);
        (void)atlas_a->sub(atlas_b);
        (void)atlas_a->mul(atlas_b);
        (void)atlas_a->div(atlas_b);
        (void)atlas_a->mod(atlas_b);
        (void)atlas_a->pow(atlas_b);
        
        std::cout << "Operators: +, -, *, /\n";
        auto result_add = (*atlas_a) + atlas_b;
        auto result_sub = (*atlas_a) - atlas_b;
        auto result_mul = (*atlas_a) * atlas_b;
        auto result_div = (*atlas_a) / atlas_b;
        
        // COMPARISONS
        print_header("11. Comparison Operations");
        std::cout << "eq(), ne(), lt(), le(), gt(), ge()\n";
        (void)atlas_a->eq(atlas_b);
        (void)atlas_a->ne(atlas_b);
        (void)atlas_a->lt(atlas_b);
        (void)atlas_a->le(atlas_b);
        (void)atlas_a->gt(atlas_b);
        (void)atlas_a->ge(atlas_b);
        
        // COMBINING
        print_header("12. Combining Atlases");
        auto comb1_df = df1->iloc(0, 5);
        auto comb2_df = df2->iloc(0, 5);
        auto comb1 = std::make_shared<Atlas>(std::vector<std::shared_ptr<DataFrame>>{comb1_df, comb2_df});
        auto comb2 = std::make_shared<Atlas>(std::vector<std::shared_ptr<DataFrame>>{comb1_df});
        
        std::cout << "append(atlas) - Append another Atlas\n";
        std::cout << "  comb1: " << comb1->ndepth() << " depths\n";
        std::cout << "  comb2: " << comb2->ndepth() << " depths\n";
        auto appended = comb1->append(comb2);
        std::cout << "  After append: " << appended->ndepth() << " depths\n";
        
        std::cout << "append_depth(df) - Add DataFrame as new depth\n";
        std::cout << "  Before: " << comb1->ndepth() << " depths\n";
        auto appended2 = comb1->append_depth(comb1_df, "new_depth");
        std::cout << "  After: " << appended2->ndepth() << " depths\n";
        
        std::cout << "concat([atlas1, atlas2]) - Static method\n";
        auto concatenated = Atlas::concat({comb1, comb2});
        std::cout << "  Result: " << concatenated->ndepth() << " depths\n";
        
        // UTILITIES
        print_header("13. Utility Methods");
        std::cout << "copy() - Create independent copy\n";
        auto original = atlas;
        auto copy = original->copy();
        std::cout << "  Copy shape: " << copy->ndepth() << "×" << copy->nrows() << "×" << copy->ncols() << "\n";
        
        std::cout << "equals(other) - Check equality\n";
        std::cout << "  original.equals(copy): " << (original->equals(copy) ? "true" : "false") << "\n";
        
        std::cout << "has_consistent_structure() - Validate\n";
        std::cout << "  " << (atlas->has_consistent_structure() ? "Consistent" : "Inconsistent") << "\n";
        
        std::cout << "rename_depths(old_to_new_map)\n";
        auto depth_names_vec = atlas->depth_names();
        std::unordered_map<std::string, std::string> rename_map;
        for (size_t i = 0; i < depth_names_vec.size() && i < 3; ++i) {
            rename_map[depth_names_vec[i]] = std::string(1, 'A' + i);
        }
        auto renamed = atlas->rename_depths(rename_map);
        std::cout << "  New names: ";
        for (const auto& n : renamed->depth_names()) std::cout << n << " ";
        std::cout << "\n";
        
        std::cout << "to_string() - String representation\n";
        std::string str = atlas->to_string();
        std::cout << "  Length: " << str.length() << " chars\n";
        
        std::cout << "to_json() - JSON representation\n";
        std::string json = atlas->to_json();
        std::cout << "  Length: " << json.length() << " chars\n";
        
        // SUMMARY
        std::cout << "\n╔══════════════════════════════════════════════════════╗\n";
        std::cout << "║             ALL ATLAS METHODS DEMONSTRATED            ║\n";
        std::cout << "╠══════════════════════════════════════════════════════╣\n";
        std::cout << "║ ✓ Construction (basic & named)                       ║\n";
        std::cout << "║ ✓ Access (at, operator[], metadata)                  ║\n";
        std::cout << "║ ✓ Slicing (head, tail, iloc, select_depths, etc)    ║\n";
        std::cout << "║ ✓ Sorting (sort_index, reindex)                     ║\n";
        std::cout << "║ ✓ Filtering (drop_depths)                            ║\n";
        std::cout << "║ ✓ Transformations (abs, sqrt, log, trig, clip)      ║\n";
        std::cout << "║ ✓ Cumulative (cumsum, cumprod, *_depth variants)    ║\n";
        std::cout << "║ ✓ Aggregations (sum, mean, std, *_depth variants)   ║\n";
        std::cout << "║ ✓ Arithmetic (add, sub, mul, div, operators)        ║\n";
        std::cout << "║ ✓ Comparisons (eq, ne, lt, le, gt, ge)              ║\n";
        std::cout << "║ ✓ Combining (append, concat)                         ║\n";
        std::cout << "║ ✓ Utilities (copy, equals, rename, to_string, etc)  ║\n";
        std::cout << "╚══════════════════════════════════════════════════════╝\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ ERROR: " << e.what() << "\n";
        return 1;
    }
}
