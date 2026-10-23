#include <iostream>
#include <vector>

#include "pluss/pluss.h"

int main() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  auto sales = DataFrame({
      {"region", Slate(pluss::make_note({"West", "West", "East", "East"}))},
      {"account", Slate(pluss::make_series(std::vector<std::int64_t>{101, 101, 201, 201}))},
      {"quarter", Slate(pluss::make_note({"Q1", "Q2", "Q1", "Q2"}))},
      {"sales", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}))},
      {"cost", Slate(pluss::make_series(std::vector<double>{4.0, 8.0, 12.0, 16.0}))},
  });

  auto indexed = sales.labels().set(DataFrame::SetLabelOptions{
      .features = {"region", "account", "quarter"},
      .drop = false,
      .verify = true,
      .sort = false,
  });
  std::cout << "Structured case levels\n";
  for (const auto& name : indexed.labels().names()) {
    std::cout << "- " << name << "\n";
  }
  std::cout << indexed.features({"sales", "cost"}).head(4) << "\n";

  auto west = indexed.case_study().pick(DataFrame::CasePick{
      .label = "region",
      .value = "West",
  });
  std::cout << "Partial level selection: region=West\n";
  std::cout << west.features({"sales", "cost"}).head(4) << "\n";

  auto one_cell = indexed.case_study().pick(DataFrame::CasePick{
      .key = {"East", "201", "Q2"},
  });
  std::cout << "Full structured key selection: East / 201 / Q2\n";
  std::cout << one_cell.features({"sales", "cost"}).head(2) << "\n";

  auto sorted = indexed.labels().sort(DataFrame::SortLabelOptions{
      .names = {"region", "account", "quarter"},
      .ascending = true,
      .nulls_last = true,
  });
  std::cout << "Sorted by structured case levels\n";
  std::cout << sorted.features({"sales", "cost"}).head(4) << "\n";

  std::vector<DataFrame::GroupSpec> group_specs{
      {"total_sales", "sales", DataFrame::GroupOp::Sum},
      {"mean_cost", "cost", DataFrame::GroupOp::Mean},
      {"n", "", DataFrame::GroupOp::Count},
  };
  auto by_region =
      indexed.group().by(DataFrame::LabelGroup{.label = "region"}).agg(group_specs);
  std::cout << "Grouped by case level: region\n";
  std::cout << by_region << "\n";

  auto renamed = indexed.labels().rename({{"account", "acct"}});
  auto reordered = renamed.labels().order({"region", "quarter", "acct"});
  auto dropped = reordered.labels().drop({"quarter"});
  std::cout << "Renamed/reordered/dropped levels\n";
  for (const auto& name : dropped.labels().names()) {
    std::cout << "- " << name << "\n";
  }

  auto reset = indexed.labels().reset(DataFrame::ResetLabelOptions{
      .drop = false,
      .names = {"region_case", "account_case", "quarter_case"},
  });
  std::cout << "Reset cases back into ordinary features\n";
  std::cout << reset.features({"region_case", "account_case", "quarter_case", "sales"}).head(4) << "\n";

  auto compact = sales.labels().set({"region", "account", "quarter"}, true);
  auto wide = compact.unstack("quarter");
  std::cout << "Unstack quarter into features\n";
  std::cout << wide << "\n";

  auto long_again = wide.stack("quarter", "sales");
  std::cout << "Stack quarter features back into structured cases\n";
  std::cout << long_again << "\n";

  auto featurewise = indexed.features({"sales", "cost"}).transform(
      [](const Slate& values) {
        return values + 1.0;
      },
      pluss::table::Axis::Feature);
  std::cout << "Feature-wise transform with Axis::Feature\n";
  std::cout << featurewise.head(4) << "\n";

  auto row_score = indexed.features({"sales", "cost"}).transform(
      [](const DataFrame& row) {
        auto sales_value = row.features("sales").as_series();
        auto cost_value = row.features("cost").as_series();
        return pluss::table::SlateValue(Slate(sales_value - cost_value));
      },
      pluss::table::Axis::Case);
  std::cout << "Row-wise transform with Axis::Case\n";
  std::cout << row_score << "\n";

  auto slatewise = indexed.features("sales").transform(
      [](const pluss::table::SlateValue& value) {
        return pluss::table::SlateValue(value.slate() * 2.0);
      });
  std::cout << "Slate element-wise transform\n";
  std::cout << slatewise << "\n";

  return 0;
}
