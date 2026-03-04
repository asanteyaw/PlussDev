#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "plot/modules/axes.h"
#include "plot/modules/camera_3d.h"
#include "plot/modules/style.h"
#include "plot/modules/trace.h"

namespace pluss::plotism {

enum class OutputFormat : std::uint8_t {
  Svg = 0,
  Png = 1,
  Pdf = 2,
};

struct RenderTarget {
  std::string_view path{};
  std::int32_t width{1200};
  std::int32_t height{800};
  OutputFormat format{OutputFormat::Svg};
};

struct Figure2DView {
  const std::string& title;
  std::int32_t rows;
  std::int32_t cols;
  const std::vector<Subplot2D>& subplots;
};

struct Figure3DView {
  const std::string& title;
  const XAxis& x_axis;
  const YAxis& y_axis;
  const ZAxis& z_axis;
  const std::array<double, 3>& box_aspect;
  const Camera3D& camera;
  const Legend& legend;
  const std::optional<Colorbar>& colorbar;
  const Style& style;
  const std::vector<Trace3D>& traces;
};

class IBackend {
public:
  virtual ~IBackend() = default;
  virtual void save(const Figure2DView& figure, const RenderTarget& target) const = 0;
  virtual void save(const Figure3DView& figure, const RenderTarget& target) const = 0;
};

std::shared_ptr<const IBackend> make_svg_backend();
std::shared_ptr<const IBackend> make_pdf_backend();
std::shared_ptr<const IBackend> make_raster_backend();
std::shared_ptr<const IBackend> make_default_backend();

} // namespace pluss::plotism
