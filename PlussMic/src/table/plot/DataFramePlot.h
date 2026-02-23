#pragma once

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string_view>

#include "table/DataFrame.h"
#include "pluss/plot/plot.h"

namespace pluss::table {

class DataFramePlotAccessor {
public:
  explicit DataFramePlotAccessor(DataFrame frame);

  pluss::plotism::Figure line(std::string_view y_feature,
                              std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::plotism::Figure line(std::initializer_list<std::string_view> y_features,
                              std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::plotism::Figure scatter(std::string_view x_feature, std::string_view y_feature) const;
  pluss::plotism::Figure bar(std::string_view y_feature,
                             std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::plotism::Figure hist(std::string_view feature, std::int64_t bins = 20) const;

private:
  DataFrame frame_{};
};

} // namespace pluss::table
