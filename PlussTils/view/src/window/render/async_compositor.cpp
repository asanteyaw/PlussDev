#include "async_compositor.h"

#include <utility>
#include <vector>

#include "pluss/tils/view/window/renderer.h"

namespace pluss::view::window::detail {

AsyncSoftwareCompositor::AsyncSoftwareCompositor()
    : worker_([this] { run_(); }) {}

AsyncSoftwareCompositor::~AsyncSoftwareCompositor() {
  {
    const std::scoped_lock lock(mutex_);
    stopping_ = true;
    pending_.clear();
  }
  wake_.notify_all();
  completed_wake_.notify_all();
  if (worker_.joinable()) {
    worker_.join();
  }
}

void AsyncSoftwareCompositor::submit(AsyncCompositorRequest request) {
  {
    const std::scoped_lock lock(mutex_);
    if (stopping_ || failed_) {
      return;
    }
    if (!request.incremental_scene.has_value()) {
      // A full scene is self-contained, so older queued full frames can be
      // discarded without breaking the worker's backing-store history.
      pending_.clear();
    }
    pending_.push_back(std::move(request));
  }
  wake_.notify_one();
}

std::optional<AsyncCompositorFrame> AsyncSoftwareCompositor::take_latest() {
  const std::scoped_lock lock(mutex_);
  if (!completed_.has_value()) {
    return std::nullopt;
  }
  auto frame = std::move(completed_);
  completed_.reset();
  return frame;
}

std::optional<AsyncCompositorFrame> AsyncSoftwareCompositor::wait_for_frame(
    const std::uint64_t minimum_frame_id,
    const std::chrono::milliseconds timeout) {
  std::unique_lock lock(mutex_);
  const bool ready = completed_wake_.wait_for(lock, timeout, [&] {
    return stopping_ || failed_ ||
           (completed_.has_value() && completed_->frame_id >= minimum_frame_id);
  });
  if (!ready || failed_ || !completed_.has_value() ||
      completed_->frame_id < minimum_frame_id) {
    return std::nullopt;
  }
  auto frame = std::move(completed_);
  completed_.reset();
  return frame;
}

bool AsyncSoftwareCompositor::has_pending_work() const {
  const std::scoped_lock lock(mutex_);
  return rendering_ || !pending_.empty();
}

bool AsyncSoftwareCompositor::failed() const {
  const std::scoped_lock lock(mutex_);
  return failed_;
}

void AsyncSoftwareCompositor::run_() {
  SoftwareRenderer renderer;
  PixelCanvas working_canvas;
  std::uint64_t rendered_frame_id = 0;

  for (;;) {
    AsyncCompositorRequest request;
    {
      std::unique_lock lock(mutex_);
      wake_.wait(lock, [&] { return stopping_ || !pending_.empty(); });
      if (stopping_) {
        return;
      }
      request = std::move(pending_.front());
      pending_.pop_front();
      rendering_ = true;
    }

    const bool same_size = working_canvas.size().width == request.canvas_size.width &&
                           working_canvas.size().height == request.canvas_size.height;
    const bool can_increment =
        request.incremental_scene.has_value() && same_size &&
        rendered_frame_id == request.base_frame_id;

    bool ok = true;
    if (!same_size) {
      ok = working_canvas.resize(request.canvas_size);
    }
    if (ok && !can_increment) {
      working_canvas.clear(request.background);
    }

    std::vector<RenderEvent> events;
    if (ok) {
      const Scene& scene =
          can_increment ? *request.incremental_scene : request.full_scene;
      ok = renderer.render(scene, working_canvas, events);
    }
    if (ok) {
      for (const auto& event : events) {
        if (event.kind == RenderEventKind::Error) {
          ok = false;
          break;
        }
      }
    }

    {
      const std::scoped_lock lock(mutex_);
      rendering_ = false;
      if (!ok) {
        failed_ = true;
        pending_.clear();
      } else {
        rendered_frame_id = request.frame_id;
        // Preserve the worker's canvas as the base for incremental damage;
        // presentation receives an immutable snapshot.
        completed_ = AsyncCompositorFrame{
            .frame_id = rendered_frame_id,
            .canvas = working_canvas,
        };
      }
    }
    completed_wake_.notify_all();
  }
}

} // namespace pluss::view::window::detail
