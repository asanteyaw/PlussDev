#pragma once

#include <string>
#include <utility>
#include <vector>

#include "plot/modules/figure.h"

namespace pluss::plotism {

inline Figure3D line3(const std::vector<double>& x,
                      const std::vector<double>& y,
                      const std::vector<double>& z,
                      std::string name = "") {
  Figure3D fig;
  fig.add_line3(x, y, z, std::move(name));
  return fig;
}

inline Figure3D line3(const std::vector<double>& x,
                      const std::vector<double>& y,
                      const std::vector<double>& z,
                      std::string name,
                      const Style& style) {
  Figure3D fig;
  fig.add_line3(x, y, z, std::move(name), style);
  return fig;
}

inline Figure3D scatter3(const std::vector<double>& x,
                         const std::vector<double>& y,
                         const std::vector<double>& z,
                         std::string name = "") {
  Figure3D fig;
  fig.add_scatter3(x, y, z, std::move(name));
  return fig;
}

inline Figure3D scatter3(const std::vector<double>& x,
                         const std::vector<double>& y,
                         const std::vector<double>& z,
                         std::string name,
                         const Style& style) {
  Figure3D fig;
  fig.add_scatter3(x, y, z, std::move(name), style);
  return fig;
}

inline Figure3D surface(const std::vector<double>& x,
                        const std::vector<double>& y,
                        const std::vector<double>& z,
                        std::string name = "") {
  Figure3D fig;
  fig.add_surface(x, y, z, std::move(name));
  return fig;
}

inline Figure3D surface(const std::vector<double>& x,
                        const std::vector<double>& y,
                        const std::vector<double>& z,
                        std::string name,
                        const Style& style) {
  Figure3D fig;
  fig.add_surface(x, y, z, std::move(name), style);
  return fig;
}

} // namespace pluss::plotism
