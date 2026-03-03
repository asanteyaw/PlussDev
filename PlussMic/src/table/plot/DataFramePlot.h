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
  pluss::plotism::Figure step(std::string_view y_feature,
                              std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::plotism::Figure step(std::initializer_list<std::string_view> y_features,
                              std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::plotism::Figure area(std::string_view y_feature,
                              std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::plotism::Figure area(std::initializer_list<std::string_view> y_features,
                              std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::plotism::Figure area_stacked(std::initializer_list<std::string_view> y_features,
                                      std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::plotism::Figure scatter(std::string_view x_feature,
                                 std::string_view y_feature,
                                 std::optional<std::string_view> hue_feature = std::nullopt) const;
  pluss::plotism::Figure bar(std::string_view y_feature,
                             std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::plotism::Figure bar(std::initializer_list<std::string_view> y_features,
                             std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::plotism::Figure bar_stacked(std::initializer_list<std::string_view> y_features,
                                     std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::plotism::Figure box(std::initializer_list<std::string_view> features = {}) const;
  pluss::plotism::Figure hist(std::string_view feature, std::int64_t bins = 20) const;
  pluss::plotism::Figure heatmap(std::initializer_list<std::string_view> features) const;
  pluss::plotism::Figure heatmap(std::string_view case_feature,
                                 std::string_view feature_feature,
                                 std::string_view value_feature,
                                 DataFrame::GroupOp op = DataFrame::GroupOp::Mean) const;
  pluss::plotism::Figure corr_heatmap(std::initializer_list<std::string_view> features = {}) const;
  pluss::plotism::Figure scatter_matrix(std::initializer_list<std::string_view> features = {}) const;
  pluss::plotism::Figure auto_plot(std::initializer_list<std::string_view> features = {}) const;

private:
  DataFrame frame_{};
};

} // namespace pluss::table
