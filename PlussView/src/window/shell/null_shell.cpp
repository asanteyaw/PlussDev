#include "pluss/view/window/shell.h"

#include <memory>
#include <utility>

namespace pluss::view::window {

bool NullWindowShell::open(const ShellConfig& config) {
  open_ = true;
  size_ = config.initial_size;
  events_.push_back(ShellEvent{ShellEventKind::Opened, size_, "null_shell_opened"});
  return true;
}

bool NullWindowShell::present(const PixelCanvas& canvas) {
  if (!open_ || canvas.empty()) {
    events_.push_back(ShellEvent{ShellEventKind::Error, size_, "null_shell_present_failed"});
    return false;
  }

  events_.push_back(ShellEvent{ShellEventKind::Presented, canvas.size(), "null_shell_present"});
  return true;
}

void NullWindowShell::close() noexcept {
  if (!open_) {
    return;
  }
  open_ = false;
  events_.push_back(ShellEvent{ShellEventKind::Closed, size_, "null_shell_closed"});
}

std::vector<ShellEvent> NullWindowShell::poll_events() {
  auto buffered = std::move(events_);
  events_.clear();
  return buffered;
}

ShellBackend NullWindowShell::backend() const noexcept {
  return ShellBackend::Null;
}

ShellCapabilities NullWindowShell::capabilities() const noexcept {
  return ShellCapabilities{
      .supports_live_present = true,
      .supports_resize_events = false,
      .supports_shortcut_events = false,
      .supports_pointer_scroll = false,
      .supports_pointer_drag = false,
      .supports_pointer_click = false,
      .supports_text_input = false,
  };
}

std::unique_ptr<IWindowShell> make_null_window_shell() {
  return std::make_unique<NullWindowShell>();
}

bool is_native_backend(const ShellBackend backend) noexcept {
  return backend != ShellBackend::None && backend != ShellBackend::Null;
}

} // namespace pluss::view::window
