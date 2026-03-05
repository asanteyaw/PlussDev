#pragma once

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string_view>

#include "table/DataFrame.h"
#include "pluss/pl.h"

namespace pluss::table {

class DataFramePlotAccessor {
public:
  explicit DataFramePlotAccessor(DataFrame frame);

  pluss::pl::Studio kind(std::string_view kind,
                         std::initializer_list<std::string_view> features = {},
                         std::optional<std::string_view> x_feature = std::nullopt,
                         std::optional<std::string_view> hue_feature = std::nullopt,
                         std::int64_t bins = 20) const;
  pluss::pl::Studio kind(std::string_view kind,
                         std::string_view feature,
                         std::optional<std::string_view> x_feature = std::nullopt,
                         std::optional<std::string_view> hue_feature = std::nullopt,
                         std::int64_t bins = 20) const;

  pluss::pl::Studio line(std::string_view y_feature,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio line(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio step(std::string_view y_feature,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio step(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio area(std::string_view y_feature,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio area(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio area_stacked(std::initializer_list<std::string_view> y_features,
                                 std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio scatter(std::string_view x_feature,
                            std::string_view y_feature,
                            std::optional<std::string_view> hue_feature = std::nullopt) const;
  pluss::pl::Studio bar(std::string_view y_feature,
                        std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio bar(std::initializer_list<std::string_view> y_features,
                        std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio bar_stacked(std::initializer_list<std::string_view> y_features,
                                std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio box(std::initializer_list<std::string_view> features = {}) const;
  pluss::pl::Studio hist(std::string_view feature, std::int64_t bins = 20) const;
  pluss::pl::Studio heatmap(std::initializer_list<std::string_view> features) const;
  pluss::pl::Studio heatmap(std::string_view case_feature,
                            std::string_view feature_feature,
                            std::string_view value_feature,
                            DataFrame::GroupOp op = DataFrame::GroupOp::Mean) const;
  pluss::pl::Studio corr_heatmap(std::initializer_list<std::string_view> features = {}) const;
  pluss::pl::Studio scatter_matrix(std::initializer_list<std::string_view> features = {}) const;
  pluss::pl::Studio auto_plot(std::initializer_list<std::string_view> features = {}) const;

private:
  DataFrame frame_{};
};

} // namespace pluss::table
