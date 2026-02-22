#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/plotism/modules/axes.h"
#include "pluss/plotism/modules/style.h"
#include "pluss/plotism/modules/trace.h"

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
  const Axis& x_axis;
  const Axis& y_axis;
  const Axis& z_axis;
  const Legend& legend;
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
std::shared_ptr<const IBackend> make_default_backend();

} // namespace pluss::plotism
