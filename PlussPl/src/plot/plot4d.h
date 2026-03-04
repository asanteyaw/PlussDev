#pragma once

#include <string>
#include <utility>

#include "plot/modules/figure.h"

namespace pluss::plotism {

inline Figure4D scatter4(const pluss::Series& x,
                         const pluss::Series& y,
                         const pluss::Series& z,
                         const pluss::Series& w,
                         std::string name = "") {
  Figure4D fig;
  fig.add_scatter4(x, y, z, w, std::move(name));
  return fig;
}

inline Figure4D scatter4(const pluss::Series& x,
                         const pluss::Series& y,
                         const pluss::Series& z,
                         const pluss::Series& w,
                         std::string name,
                         const Style& style) {
  Figure4D fig;
  fig.add_scatter4(x, y, z, w, std::move(name), style);
  return fig;
}

inline Figure4D line4(const pluss::Series& x,
                      const pluss::Series& y,
                      const pluss::Series& z,
                      const pluss::Series& w,
                      std::string name = "") {
  Figure4D fig;
  fig.add_line4(x, y, z, w, std::move(name));
  return fig;
}

inline Figure4D line4(const pluss::Series& x,
                      const pluss::Series& y,
                      const pluss::Series& z,
                      const pluss::Series& w,
                      std::string name,
                      const Style& style) {
  Figure4D fig;
  fig.add_line4(x, y, z, w, std::move(name), style);
  return fig;
}

} // namespace pluss::plotism
