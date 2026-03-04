#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "plot/modules/figure.h"

namespace pluss::plotism {

inline Figure3D line3(const pluss::Series& x,
                      const pluss::Series& y,
                      const pluss::Series& z,
                      std::string name = "") {
  Figure3D fig;
  fig.add_line3(x, y, z, std::move(name));
  return fig;
}

inline Figure3D line3(const pluss::Series& x,
                      const pluss::Series& y,
                      const pluss::Series& z,
                      std::string name,
                      const Style& style) {
  Figure3D fig;
  fig.add_line3(x, y, z, std::move(name), style);
  return fig;
}

inline Figure3D scatter3(const pluss::Series& x,
                         const pluss::Series& y,
                         const pluss::Series& z,
                         std::string name = "") {
  Figure3D fig;
  fig.add_scatter3(x, y, z, std::move(name));
  return fig;
}

inline Figure3D scatter3(const pluss::Series& x,
                         const pluss::Series& y,
                         const pluss::Series& z,
                         std::string name,
                         const Style& style) {
  Figure3D fig;
  fig.add_scatter3(x, y, z, std::move(name), style);
  return fig;
}

inline Figure3D surf(const pluss::Series& x,
                     const pluss::Series& y,
                     const pluss::Series& z,
                     std::string name = "") {
  Figure3D fig;
  fig.add_surface(x, y, z, std::move(name)).surface_filled(true).surface_wireframe(false);
  return fig;
}

inline Figure3D surf(const pluss::Series& x,
                     const pluss::Series& y,
                     const pluss::Series& z,
                     std::string name,
                     const Style& style) {
  Figure3D fig;
  fig.add_surface(x, y, z, std::move(name), style).surface_filled(true).surface_wireframe(false);
  return fig;
}

inline Figure3D surface(const pluss::Series& x,
                        const pluss::Series& y,
                        const pluss::Series& z,
                        std::string name = "") {
  return surf(x, y, z, std::move(name));
}

inline Figure3D surface(const pluss::Series& x,
                        const pluss::Series& y,
                        const pluss::Series& z,
                        std::string name,
                        const Style& style) {
  return surf(x, y, z, std::move(name), style);
}

inline Figure3D mesh(const pluss::Series& x,
                     const pluss::Series& y,
                     const pluss::Series& z,
                     std::string name = "") {
  Figure3D fig;
  fig.add_surface(x, y, z, std::move(name)).mesh_mode();
  return fig;
}

inline Figure3D mesh(const pluss::Series& x,
                     const pluss::Series& y,
                     const pluss::Series& z,
                     std::string name,
                     const Style& style) {
  Figure3D fig;
  fig.add_surface(x, y, z, std::move(name), style).mesh_mode();
  return fig;
}

inline Figure3D waterfall(const pluss::Series& x,
                          const pluss::Series& y,
                          const pluss::Series& z,
                          std::string name = "") {
  Figure3D fig;
  fig.add_surface(x, y, z, std::move(name)).waterfall_mode();
  return fig;
}

inline Figure3D waterfall(const pluss::Series& x,
                          const pluss::Series& y,
                          const pluss::Series& z,
                          std::string name,
                          const Style& style) {
  Figure3D fig;
  fig.add_surface(x, y, z, std::move(name), style).waterfall_mode();
  return fig;
}

inline Figure3D meshz(const pluss::Series& x,
                      const pluss::Series& y,
                      const pluss::Series& z,
                      std::string name = "") {
  Figure3D fig;
  fig.add_surface(x, y, z, std::move(name)).meshz_mode();
  return fig;
}

inline Figure3D meshz(const pluss::Series& x,
                      const pluss::Series& y,
                      const pluss::Series& z,
                      std::string name,
                      const Style& style) {
  Figure3D fig;
  fig.add_surface(x, y, z, std::move(name), style).meshz_mode();
  return fig;
}

inline Figure3D surfc(const pluss::Series& x,
                      const pluss::Series& y,
                      const pluss::Series& z,
                      std::string name = "") {
  Figure3D fig;
  fig.add_surface(x, y, z, std::move(name)).surfc_mode();
  return fig;
}

inline Figure3D meshc(const pluss::Series& x,
                      const pluss::Series& y,
                      const pluss::Series& z,
                      std::string name = "") {
  Figure3D fig;
  fig.add_surface(x, y, z, std::move(name)).meshc_mode();
  return fig;
}

inline Figure3D contour3(const pluss::Series& x,
                         const pluss::Series& y,
                         const pluss::Series& z,
                         std::int32_t levels = 8,
                         std::string name = "") {
  Figure3D fig;
  fig.add_contour3(x, y, z, std::move(name), levels);
  return fig;
}

inline Figure3D contour3(const pluss::Series& x,
                         const pluss::Series& y,
                         const pluss::Series& z,
                         std::int32_t levels,
                         std::string name,
                         const Style& style) {
  Figure3D fig;
  fig.add_contour3(x, y, z, std::move(name), levels, style);
  return fig;
}

inline Figure3D surfl(const pluss::Series& x,
                      const pluss::Series& y,
                      const pluss::Series& z,
                      std::string name = "") {
  Figure3D fig;
  fig.add_surface(x, y, z, std::move(name)).surfl_mode();
  return fig;
}

inline Figure3D surfl(const pluss::Series& x,
                      const pluss::Series& y,
                      const pluss::Series& z,
                      double light_azimuth_deg,
                      double light_elevation_deg,
                      std::string name = "") {
  Figure3D fig;
  fig.add_surface(x, y, z, std::move(name)).surfl_mode(light_azimuth_deg, light_elevation_deg);
  return fig;
}

inline Figure3D fill3(const pluss::Series& x,
                      const pluss::Series& y,
                      const pluss::Series& z,
                      std::string name = "") {
  Figure3D fig;
  fig.add_fill3(x, y, z, std::move(name));
  return fig;
}

inline Figure3D fill3(const pluss::Series& x,
                      const pluss::Series& y,
                      const pluss::Series& z,
                      std::string name,
                      const Style& style) {
  Figure3D fig;
  fig.add_fill3(x, y, z, std::move(name), style);
  return fig;
}

} // namespace pluss::plotism
