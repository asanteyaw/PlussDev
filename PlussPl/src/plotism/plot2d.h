#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "pluss/plotism/modules/figure.h"

namespace pluss::plotism {

inline Figure2D line(const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_line(y, std::move(name));
  return fig;
}

inline Figure2D line(const std::vector<double>& x, const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_line(x, y, std::move(name));
  return fig;
}

inline Figure2D scatter(const std::vector<double>& x, const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_scatter(x, y, std::move(name));
  return fig;
}

inline Figure2D bar(const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_bar(y, std::move(name));
  return fig;
}

inline Figure2D bar(const std::vector<double>& x, const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_bar(x, y, std::move(name));
  return fig;
}

inline Figure2D hist(const std::vector<double>& values, std::int64_t bins = 20, std::string name = "") {
  Figure2D fig;
  fig.add_hist(values, bins, std::move(name));
  return fig;
}

} // namespace pluss::plotism
