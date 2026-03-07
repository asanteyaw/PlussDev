#include "pluss/view/window/window_registry.h"

#include <algorithm>
#include <ranges>
#include <utility>
#include <vector>

namespace pluss::view::window {

WindowRegistry& WindowRegistry::instance() {
  static WindowRegistry registry;
  return registry;
}

WindowSessionId WindowRegistry::create_line_plot_session(LinePlotController controller,
                                                         LinePlotWindowSessionConfig config,
                                                         std::unique_ptr<IWindowShell> shell) {
  auto session = std::make_unique<LinePlotWindowSession>(std::move(controller),
                                                         std::move(config),
                                                         std::move(shell));
  return register_session(std::move(session));
}

WindowSessionId WindowRegistry::create_table_session(TableController controller,
                                                     LinePlotWindowSessionConfig config,
                                                     std::unique_ptr<IWindowShell> shell) {
  auto session = std::make_unique<TableWindowSession>(std::move(controller),
                                                      std::move(config),
                                                      std::move(shell));
  return register_session(std::move(session));
}

WindowSessionId WindowRegistry::register_session(std::unique_ptr<IWindowSession> session) {
  if (!session) {
    return 0;
  }

  std::scoped_lock lock(mutex_);
  const WindowSessionId id = next_id_++;
  sessions_.emplace(id, std::move(session));
  return id;
}

bool WindowRegistry::open(const WindowSessionId id) {
  std::scoped_lock lock(mutex_);
  const auto it = sessions_.find(id);
  return it != sessions_.end() && it->second->open();
}

bool WindowRegistry::enqueue(const WindowSessionId id, WindowCommand command) {
  std::scoped_lock lock(mutex_);
  const auto it = sessions_.find(id);
  return it != sessions_.end() && it->second->enqueue(std::move(command));
}

bool WindowRegistry::tick(const WindowSessionId id) {
  std::scoped_lock lock(mutex_);
  const auto it = sessions_.find(id);
  if (it == sessions_.end()) {
    return false;
  }
  return it->second->tick();
}

std::size_t WindowRegistry::tick_all() {
  std::scoped_lock lock(mutex_);
  std::size_t ticked = 0;
  for (auto& [id, session] : sessions_) {
    (void)id;
    if (session->state() == WindowSessionState::Open && session->tick()) {
      ++ticked;
    }
  }
  return ticked;
}

std::size_t WindowRegistry::prune_closed() {
  std::scoped_lock lock(mutex_);
  const auto before = sessions_.size();
  std::erase_if(sessions_, [](const auto& kv) {
    const auto state = kv.second->state();
    return state == WindowSessionState::Closed || state == WindowSessionState::Failed;
  });
  return before - sessions_.size();
}

bool WindowRegistry::close(const WindowSessionId id) {
  std::scoped_lock lock(mutex_);
  const auto it = sessions_.find(id);
  if (it == sessions_.end()) {
    return false;
  }
  it->second->close();
  return true;
}

bool WindowRegistry::remove(const WindowSessionId id) {
  std::scoped_lock lock(mutex_);
  return sessions_.erase(id) > 0;
}

void WindowRegistry::close_all() {
  std::scoped_lock lock(mutex_);
  for (auto& [id, session] : sessions_) {
    (void)id;
    session->close();
  }
}

PixelCanvas WindowRegistry::canvas(const WindowSessionId id) const {
  std::scoped_lock lock(mutex_);
  const auto it = sessions_.find(id);
  if (it == sessions_.end()) {
    return PixelCanvas{};
  }
  return it->second->canvas();
}

bool WindowRegistry::save_canvas(const WindowSessionId id, const std::string_view path) const {
  PixelCanvas snapshot;
  {
    std::scoped_lock lock(mutex_);
    const auto it = sessions_.find(id);
    if (it == sessions_.end()) {
      return false;
    }
    snapshot = it->second->canvas();
  }
  return window::save_canvas(snapshot, path);
}

bool WindowRegistry::save_canvas(const WindowSessionId id,
                                 const std::string_view path,
                                 const CanvasExportFormat format) const {
  PixelCanvas snapshot;
  {
    std::scoped_lock lock(mutex_);
    const auto it = sessions_.find(id);
    if (it == sessions_.end()) {
      return false;
    }
    snapshot = it->second->canvas();
  }
  return window::save_canvas(snapshot, path, format);
}

std::vector<WindowSessionEvent> WindowRegistry::events(const WindowSessionId id) const {
  std::scoped_lock lock(mutex_);
  const auto it = sessions_.find(id);
  if (it == sessions_.end()) {
    return {};
  }
  return it->second->events();
}

std::vector<WindowSessionSnapshot> WindowRegistry::snapshots() const {
  std::scoped_lock lock(mutex_);
  std::vector<WindowSessionSnapshot> rows;
  rows.reserve(sessions_.size());
  for (const auto& [id, session] : sessions_) {
    rows.push_back(WindowSessionSnapshot{
        .id = id,
        .state = session->state(),
        .backend = session->shell_backend(),
        .capabilities = session->shell_capabilities(),
        .size = session->canvas().size(),
        .frame_id = session->frame_id(),
        .queued_commands = session->queued_command_count(),
        .title = session->title(),
    });
  }
  std::ranges::sort(rows, {}, &WindowSessionSnapshot::id);
  return rows;
}

std::size_t WindowRegistry::size() const {
  std::scoped_lock lock(mutex_);
  return sessions_.size();
}

} // namespace pluss::view::window
