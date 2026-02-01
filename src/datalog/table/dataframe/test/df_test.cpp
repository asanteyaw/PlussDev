#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <pluss/pluss.h>
#include <pluss/datalog/table/dataframe/dataframe.h>
#include <pluss/datalog/table/thread/thread.h>
#include <filesystem>

using namespace pluss;
using namespace pluss::table;

int main() {
    // Basic create / access
    std::unordered_map<std::string, pluss::Series> data;
    data["a"] = pluss::value({1,2,3});
    data["b"] = pluss::value({4,5,6});

    auto df = std::make_shared<DataFrame>(data);
    auto cols = df->columns();
    assert(cols.size() == 2 && "expected two columns");

    // set a column with Thread (Series)
    at::Tensor t = at::tensor({1,2,3}, at::dtype(at::kInt));
    pluss::Series s(t);
    df = df->set_col("c", Thread(std::move(s), "c"));
    {
        auto cols_after = df->columns();
        assert(std::count(cols_after.begin(), cols_after.end(), "c") == 1);
    }

    // get column as Thread and as Series
    Thread th = df->get_col("c");
    assert(th.is_series());
    pluss::Series s2 = th.as_series();
    assert(s2.numel() == 3);

    // IO test with bank-full.csv (skip if missing)
    // tests run from the build/ directory, so data is ../data/
    auto path = std::filesystem::path("../data/bank-full.csv");
    if (std::filesystem::exists(path)) {
        auto df2 = DataFrame::load("csv", path.string());
        assert(df2 != nullptr);
        auto cols2 = df2->columns();
        assert(!cols2.empty());
        auto vals = df2->values();
        assert(vals.size() == cols2.size());
        auto small = df2->iloc(0, std::min<int64_t>(5, vals.empty() ? 0 : vals[0].size()));
        assert(small != nullptr);
    } else {
        std::cout << "data/bank-full.csv not found; skipping IO test\n";
    }

    // Thread conversions
    at::Tensor tf = at::tensor({1.0f,2.0f,3.0f});
    pluss::Series sf(tf);
    Thread th2(sf, "vals");
    assert(th2.is_series());
    assert(th2.size() == 3);

    std::cout << "df_test: all assertions passed\n";
    return 0;
}
