#include <cassert>
#include <iostream>
#include <string>

#include <torch/torch.h>

#include <pluss/pluss.h>

static void test_width_truncation_and_column_ellipsis() {
    using pluss::table::DataFrame;
    using pluss::table::Thread;

    // Build a moderately wide DataFrame with long strings.
    auto df = std::make_shared<DataFrame>();

    const int64_t n = 8;

    pluss::Series a = pluss::value({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0});
    pluss::Series b = pluss::value({10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0});

    pluss::Note text = pluss::make_note({
        "this is a very very long string cell value that should be truncated",
        "short",
        "another extremely long value that should not break your terminal width",
        "alpha",
        "beta",
        "gamma",
        "delta",
        "epsilon"
    });

    // Add more columns to force truncation.
    df = df->set_col("col_A_really_long_name", Thread(std::move(a), "col_A_really_long_name"));
    df = df->set_col("col_B_really_long_name", Thread(std::move(b), "col_B_really_long_name"));
    df = df->set_col("text_column_with_a_long_name", Thread(std::move(text), "text_column_with_a_long_name"));

    // Add a couple more numeric columns.
    df = df->set_col("C1", Thread(pluss::value({1,2,3,4,5,6,7,8}), "C1"));
    df = df->set_col("C2", Thread(pluss::value({8,7,6,5,4,3,2,1}), "C2"));

    pluss::table::io::DisplayOptions opt;
    opt.max_width = 60;   // force narrow output
    opt.max_rows = n;
    opt.head_rows = n;
    opt.tail_rows = 0;
    opt.show_summary = true;

    const std::string out = pluss::table::io::to_string(*df, opt);

    // We expect the output to include an ellipsis column when it can't fit.
    assert(out.find("...") != std::string::npos);

    // We expect the summary line.
    assert(out.find("rows x") != std::string::npos);

    // Basic sanity: index + at least one header.
    assert(out.find("col_A") != std::string::npos);
}

static void test_row_truncation_head_tail() {
    using pluss::table::DataFrame;
    using pluss::table::Thread;

    auto df = std::make_shared<DataFrame>();

    // 50 rows
    std::vector<int64_t> vals;
    vals.reserve(50);
    for (int64_t i = 0; i < 50; ++i) vals.push_back(i);

    df = df->set_col("x", Thread(pluss::from_torch(torch::tensor(vals)), "x"));

    pluss::table::io::DisplayOptions opt;
    opt.max_width = 80;
    opt.max_rows = 10;
    opt.head_rows = 5;
    opt.tail_rows = 5;
    opt.show_summary = true;

    const std::string out = pluss::table::io::to_string(*df, opt);

    // Should contain the ellipsis row marker.
    assert(out.find("...\n") != std::string::npos || out.find(" ... ") != std::string::npos);

    // Should contain first and last indices.
    assert(out.find("0") != std::string::npos);
    assert(out.find("49") != std::string::npos);

    // Summary present.
    assert(out.find("[50 rows x") != std::string::npos);
}

int main() {
    std::cout << "Running DataFrame display tests..." << std::endl;
    test_width_truncation_and_column_ellipsis();
    test_row_truncation_head_tail();
    std::cout << "DataFrame display tests passed!" << std::endl;
    return 0;
}
