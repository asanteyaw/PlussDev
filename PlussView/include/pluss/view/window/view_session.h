#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "pluss/view/window/plot.h"
#include "pluss/view/window/runtime.h"
#include "pluss/view/window/shell.h"
#include "pluss/view/window/table.h"

namespace pluss::view::window {

enum class WindowSessionState : std::uint8_t {
  Idle = 0,
  Open = 1,
  Closed = 2,
  Failed = 3,
};

enum class WindowSessionEventKind : std::uint8_t {
  Opened = 0,
  FrameCommitted = 1,
  Resized = 2,
  Closed = 3,
  Warning = 4,
  Error = 5,
};

struct WindowSessionEvent {
  WindowSessionEventKind kind{WindowSessionEventKind::Opened};
  std::uint64_t frame_id{0};
  SizeI size{};
  std::string note{};
};

enum class WindowCommandKind : std::uint8_t {
  Pan = 0,
  Zoom = 1,
  ResetView = 2,
  ReconfigureCanvas = 3,
  TableNextPage = 4,
  TablePreviousPage = 5,
  TableSetOffset = 6,
  Close = 7,
};

struct WindowCommand {
  WindowCommandKind kind{WindowCommandKind::ResetView};
  LinePlotInteraction interaction{};
  TableInteraction table_interaction{};
  RuntimeConfig runtime_config{};
};

struct WindowInputPolicy {
  double key_pan_step_ratio{0.10};
  double key_zoom_factor{1.15};
  double scroll_zoom_factor{1.10};
  bool invert_scroll_zoom{false};
};

struct LinePlotWindowSessionConfig {
  RuntimeConfig runtime{};
  ShellConfig shell{"PlussView Session", SizeI{1280, 760}};
  ShellSelectionPolicy shell_policy{ShellSelectionPolicy::NativePreferred};
  WindowInputPolicy input{};
};

class IWindowSession {
 public:
  virtual ~IWindowSession() = default;
  [[nodiscard]] virtual bool open() = 0;
  [[nodiscard]] virtual bool tick() = 0;
  [[nodiscard]] virtual bool enqueue(WindowCommand command) = 0;
  virtual void close() noexcept = 0;

  [[nodiscard]] virtual WindowSessionState state() const noexcept = 0;
  [[nodiscard]] virtual const PixelCanvas& canvas() const noexcept = 0;
  [[nodiscard]] virtual const std::vector<WindowSessionEvent>& events() const noexcept = 0;
  [[nodiscard]] virtual std::uint64_t frame_id() const noexcept = 0;
  [[nodiscard]] virtual std::size_t queued_command_count() const noexcept = 0;
  [[nodiscard]] virtual const std::string& title() const noexcept = 0;
  [[nodiscard]] virtual ShellBackend shell_backend() const noexcept = 0;
  [[nodiscard]] virtual ShellCapabilities shell_capabilities() const noexcept = 0;
};

class LinePlotWindowSession final : public IWindowSession {
 public:
  explicit LinePlotWindowSession(LinePlotController controller,
                                 LinePlotWindowSessionConfig config = {},
                                 std::unique_ptr<IWindowShell> shell = {});

  [[nodiscard]] bool open() override;
  [[nodiscard]] bool tick() override;
  [[nodiscard]] bool run_steps(std::size_t max_steps);
  [[nodiscard]] bool enqueue(WindowCommand command) override;
  void close() noexcept override;

  [[nodiscard]] WindowSessionState state() const noexcept override;
  [[nodiscard]] const PixelCanvas& canvas() const noexcept override;
  [[nodiscard]] const std::vector<WindowSessionEvent>& events() const noexcept override;
  [[nodiscard]] std::uint64_t frame_id() const noexcept override;
  [[nodiscard]] std::size_t queued_command_count() const noexcept override;
  [[nodiscard]] const std::string& title() const noexcept override;
  [[nodiscard]] ShellBackend shell_backend() const noexcept override;
  [[nodiscard]] ShellCapabilities shell_capabilities() const noexcept override;

 private:
  [[nodiscard]] bool render_and_present_(std::string note);
  void push_event_(WindowSessionEventKind kind, std::string note = {});

  LinePlotController controller_;
  LinePlotWindowSessionConfig config_{};
  ViewerRuntime runtime_;
  std::unique_ptr<IWindowShell> shell_{};
  WindowSessionState state_{WindowSessionState::Idle};
  std::vector<WindowCommand> commands_{};
  std::vector<WindowSessionEvent> events_{};
  bool dirty_{true};
};

class TableWindowSession final : public IWindowSession {
 public:
  explicit TableWindowSession(TableController controller,
                              LinePlotWindowSessionConfig config = {},
                              std::unique_ptr<IWindowShell> shell = {});

  [[nodiscard]] bool open() override;
  [[nodiscard]] bool tick() override;
  [[nodiscard]] bool enqueue(WindowCommand command) override;
  void close() noexcept override;

  [[nodiscard]] WindowSessionState state() const noexcept override;
  [[nodiscard]] const PixelCanvas& canvas() const noexcept override;
  [[nodiscard]] const std::vector<WindowSessionEvent>& events() const noexcept override;
  [[nodiscard]] std::uint64_t frame_id() const noexcept override;
  [[nodiscard]] std::size_t queued_command_count() const noexcept override;
  [[nodiscard]] const std::string& title() const noexcept override;
  [[nodiscard]] ShellBackend shell_backend() const noexcept override;
  [[nodiscard]] ShellCapabilities shell_capabilities() const noexcept override;

 private:
  [[nodiscard]] bool render_and_present_(std::string note);
  void push_event_(WindowSessionEventKind kind, std::string note = {});

  TableController controller_;
  LinePlotWindowSessionConfig config_{};
  ViewerRuntime runtime_;
  std::unique_ptr<IWindowShell> shell_{};
  WindowSessionState state_{WindowSessionState::Idle};
  std::vector<WindowCommand> commands_{};
  std::vector<WindowSessionEvent> events_{};
  bool dirty_{true};
};

} // namespace pluss::view::window
