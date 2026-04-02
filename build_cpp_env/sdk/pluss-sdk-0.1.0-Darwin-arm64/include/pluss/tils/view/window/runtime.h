#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "pluss/tils/view/window/renderer.h"

namespace pluss::view::window {

enum class RuntimeState : std::uint8_t {
  Idle = 0,
  Ready = 1,
  Rendering = 2,
  Failed = 3,
};

enum class RuntimeEventKind : std::uint8_t {
  Initialized = 0,
  Reconfigured = 1,
  SceneSubmitted = 2,
  FrameRendered = 3,
  Warning = 4,
  Error = 5,
};

struct RuntimeEvent {
  RuntimeEventKind kind{RuntimeEventKind::Initialized};
  std::uint64_t frame_id{0};
  std::string note{};
};

struct RuntimeConfig {
  SizeI canvas_size{1280, 760};
  Color background{Color::rgba(248, 248, 248, 255)};
};

class ViewerRuntime {
 public:
  explicit ViewerRuntime(RuntimeConfig config = {},
                         std::unique_ptr<IRenderer> renderer = make_software_renderer());

  [[nodiscard]] RuntimeState state() const noexcept;
  [[nodiscard]] std::uint64_t frame_id() const noexcept;
  [[nodiscard]] const PixelCanvas& canvas() const noexcept;
  [[nodiscard]] const std::vector<RuntimeEvent>& events() const noexcept;

  [[nodiscard]] bool configure(RuntimeConfig config);
  [[nodiscard]] bool submit(const Scene& scene);

 private:
  void push_event_(RuntimeEventKind kind, std::string note = {});

  RuntimeConfig config_{};
  RuntimeState state_{RuntimeState::Idle};
  std::uint64_t frame_id_{0};
  PixelCanvas canvas_{};
  std::unique_ptr<IRenderer> renderer_{};
  std::vector<RuntimeEvent> events_{};
};

} // namespace pluss::view::window
