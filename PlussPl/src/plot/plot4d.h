#pragma once

#include <string>
#include <utility>
#include <vector>

#include "plot/modules/figure.h"

namespace pluss::plotism {

inline Figure4D scatter4(const std::vector<double>& x,
                         const std::vector<double>& y,
                         const std::vector<double>& z,
                         const std::vector<double>& w,
                         std::string name = "") {
  Figure4D fig;
  fig.add_scatter4(x, y, z, w, std::move(name));
  return fig;
}

inline Figure4D scatter4(const std::vector<double>& x,
                         const std::vector<double>& y,
                         const std::vector<double>& z,
                         const std::vector<double>& w,
                         std::string name,
                         const Style& style) {
  Figure4D fig;
  fig.add_scatter4(x, y, z, w, std::move(name), style);
  return fig;
}

inline Figure4D line4(const std::vector<double>& x,
                      const std::vector<double>& y,
                      const std::vector<double>& z,
                      const std::vector<double>& w,
                      std::string name = "") {
  Figure4D fig;
  fig.add_line4(x, y, z, w, std::move(name));
  return fig;
}

inline Figure4D line4(const std::vector<double>& x,
                      const std::vector<double>& y,
                      const std::vector<double>& z,
                      const std::vector<double>& w,
                      std::string name,
                      const Style& style) {
  Figure4D fig;
  fig.add_line4(x, y, z, w, std::move(name), style);
  return fig;
}

} // namespace pluss::plotism
