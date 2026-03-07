#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "pluss/view/window/canvas.h"
#include "pluss/view/window/scene.h"

namespace pluss::view::window {

enum class RenderEventKind : std::uint8_t {
  FrameStarted = 0,
  FrameFinished = 1,
  UnsupportedCommand = 2,
  Error = 3,
};

struct RenderEvent {
  RenderEventKind kind{RenderEventKind::FrameStarted};
  std::string note{};
};

class IRenderer {
 public:
  virtual ~IRenderer() = default;

  virtual bool render(const Scene& scene, PixelCanvas& canvas, std::vector<RenderEvent>& events) = 0;
};

class SoftwareRenderer final : public IRenderer {
 public:
  bool render(const Scene& scene, PixelCanvas& canvas, std::vector<RenderEvent>& events) override;
};

[[nodiscard]] std::unique_ptr<IRenderer> make_software_renderer();

} // namespace pluss::view::window
