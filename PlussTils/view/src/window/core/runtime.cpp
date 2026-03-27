#include "pluss/tils/view/window/runtime.h"

#include <utility>

namespace pluss::view::window {

ViewerRuntime::ViewerRuntime(RuntimeConfig config, std::unique_ptr<IRenderer> renderer)
    : config_(std::move(config)), renderer_(std::move(renderer)) {
  if (!renderer_) {
    renderer_ = make_software_renderer();
  }

  const bool ok = canvas_.resize(config_.canvas_size);
  if (!ok) {
    state_ = RuntimeState::Failed;
    push_event_(RuntimeEventKind::Error, "invalid_canvas_size");
    return;
  }

  canvas_.clear(config_.background);
  state_ = RuntimeState::Ready;
  push_event_(RuntimeEventKind::Initialized, "window_runtime_initialized");
}

RuntimeState ViewerRuntime::state() const noexcept {
  return state_;
}

std::uint64_t ViewerRuntime::frame_id() const noexcept {
  return frame_id_;
}

const PixelCanvas& ViewerRuntime::canvas() const noexcept {
  return canvas_;
}

const std::vector<RuntimeEvent>& ViewerRuntime::events() const noexcept {
  return events_;
}

bool ViewerRuntime::configure(RuntimeConfig config) {
  config_ = std::move(config);
  if (!canvas_.resize(config_.canvas_size)) {
    state_ = RuntimeState::Failed;
    push_event_(RuntimeEventKind::Error, "invalid_canvas_size");
    return false;
  }

  canvas_.clear(config_.background);
  state_ = RuntimeState::Ready;
  push_event_(RuntimeEventKind::Reconfigured, "window_runtime_reconfigured");
  return true;
}

bool ViewerRuntime::submit(const Scene& scene) {
  if (state_ == RuntimeState::Failed || !renderer_) {
    push_event_(RuntimeEventKind::Error, "runtime_not_ready");
    return false;
  }

  state_ = RuntimeState::Rendering;
  push_event_(RuntimeEventKind::SceneSubmitted, "scene_submitted");

  std::vector<RenderEvent> render_events;
  const bool ok = renderer_->render(scene, canvas_, render_events);
  for (const auto& event : render_events) {
    if (event.kind == RenderEventKind::UnsupportedCommand) {
      push_event_(RuntimeEventKind::Warning, event.note);
      continue;
    }
    if (event.kind == RenderEventKind::Error) {
      push_event_(RuntimeEventKind::Error, event.note);
      state_ = RuntimeState::Failed;
      return false;
    }
  }

  if (!ok) {
    state_ = RuntimeState::Failed;
    push_event_(RuntimeEventKind::Error, "render_failed");
    return false;
  }

  ++frame_id_;
  state_ = RuntimeState::Ready;
  push_event_(RuntimeEventKind::FrameRendered, "frame_rendered");
  return true;
}

void ViewerRuntime::push_event_(RuntimeEventKind kind, std::string note) {
  events_.push_back(RuntimeEvent{kind, frame_id_, std::move(note)});
}

} // namespace pluss::view::window
