#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "pluss/view/window/export.h"
#include "pluss/view/window/view_session.h"

namespace pluss::view::window {

using WindowSessionId = std::uint64_t;

struct WindowSessionSnapshot {
  WindowSessionId id{0};
  WindowSessionState state{WindowSessionState::Idle};
  ShellBackend backend{ShellBackend::None};
  ShellCapabilities capabilities{};
  SizeI size{};
  std::uint64_t frame_id{0};
  std::size_t queued_commands{0};
  std::string title{};
};

class WindowRegistry {
 public:
  static WindowRegistry& instance();

  [[nodiscard]] WindowSessionId create_line_plot_session(LinePlotController controller,
                                                         LinePlotWindowSessionConfig config = {},
                                                         std::unique_ptr<IWindowShell> shell = {});
  [[nodiscard]] WindowSessionId create_table_session(TableController controller,
                                                     LinePlotWindowSessionConfig config = {},
                                                     std::unique_ptr<IWindowShell> shell = {});
  [[nodiscard]] WindowSessionId register_session(std::unique_ptr<IWindowSession> session);

  [[nodiscard]] bool open(WindowSessionId id);
  [[nodiscard]] bool enqueue(WindowSessionId id, WindowCommand command);
  [[nodiscard]] bool tick(WindowSessionId id);
  [[nodiscard]] std::size_t tick_all();
  [[nodiscard]] std::size_t prune_closed();
  [[nodiscard]] bool close(WindowSessionId id);
  [[nodiscard]] bool remove(WindowSessionId id);
  void close_all();

  [[nodiscard]] PixelCanvas canvas(WindowSessionId id) const;
  [[nodiscard]] bool save_canvas(WindowSessionId id, std::string_view path) const;
  [[nodiscard]] bool save_canvas(WindowSessionId id,
                                 std::string_view path,
                                 CanvasExportFormat format) const;
  [[nodiscard]] std::vector<WindowSessionEvent> events(WindowSessionId id) const;
  [[nodiscard]] std::vector<WindowSessionSnapshot> snapshots() const;
  [[nodiscard]] std::size_t size() const;

 private:
  WindowRegistry() = default;

  mutable std::mutex mutex_{};
  WindowSessionId next_id_{1};
  std::unordered_map<WindowSessionId, std::unique_ptr<IWindowSession>> sessions_{};
};

} // namespace pluss::view::window
