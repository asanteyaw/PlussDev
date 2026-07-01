#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>

#include "pluss/tils/view/window/canvas.h"
#include "pluss/tils/view/window/scene.h"

namespace pluss::view::window::detail {

struct AsyncCompositorRequest {
  std::uint64_t frame_id{0};
  std::uint64_t base_frame_id{0};
  SizeI canvas_size{};
  Color background{Color::rgba(248, 248, 248, 255)};
  Scene full_scene{};
  std::optional<Scene> incremental_scene{};
};

struct AsyncCompositorFrame {
  std::uint64_t frame_id{0};
  PixelCanvas canvas{};
};

class AsyncSoftwareCompositor {
 public:
  AsyncSoftwareCompositor();
  ~AsyncSoftwareCompositor();

  AsyncSoftwareCompositor(const AsyncSoftwareCompositor&) = delete;
  AsyncSoftwareCompositor& operator=(const AsyncSoftwareCompositor&) = delete;

  void submit(AsyncCompositorRequest request);
  [[nodiscard]] std::optional<AsyncCompositorFrame> take_latest();
  [[nodiscard]] std::optional<AsyncCompositorFrame> wait_for_frame(
      std::uint64_t minimum_frame_id,
      std::chrono::milliseconds timeout);
  [[nodiscard]] bool has_pending_work() const;
  [[nodiscard]] bool failed() const;

 private:
  void run_();

  mutable std::mutex mutex_{};
  std::condition_variable wake_{};
  std::condition_variable completed_wake_{};
  bool stopping_{false};
  bool rendering_{false};
  bool failed_{false};
  std::deque<AsyncCompositorRequest> pending_{};
  std::optional<AsyncCompositorFrame> completed_{};
  std::thread worker_{};
};

} // namespace pluss::view::window::detail
