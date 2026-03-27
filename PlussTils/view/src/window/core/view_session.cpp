#include "pluss/tils/view/window/view_session.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

namespace pluss::view::window {

namespace {

[[nodiscard]] double normalized_pan_step(const double raw) {
  if (!std::isfinite(raw)) {
    return 0.1;
  }
  return std::clamp(std::abs(raw), 0.005, 0.95);
}

[[nodiscard]] double normalized_zoom_factor(const double raw, const double fallback) {
  if (!std::isfinite(raw) || raw <= 0.0) {
    return fallback;
  }
  return std::clamp(raw, 1.01, 8.0);
}

[[nodiscard]] bool append_shortcut_commands(const bool table_mode,
                                            const std::string& note,
                                            const WindowInputPolicy& policy,
                                            std::vector<WindowCommand>& emitted_commands) {
  if (table_mode) {
    if (note == "next_page") {
      emitted_commands.push_back(WindowCommand{.kind = WindowCommandKind::TableNextPage});
      return true;
    }
    if (note == "previous_page") {
      emitted_commands.push_back(WindowCommand{.kind = WindowCommandKind::TablePreviousPage});
      return true;
    }
    return false;
  }

  const double pan_step = normalized_pan_step(policy.key_pan_step_ratio);
  const double zoom_step = normalized_zoom_factor(policy.key_zoom_factor, 1.15);

  if (note == "zoom_in") {
    emitted_commands.push_back(WindowCommand{
        .kind = WindowCommandKind::Zoom,
        .interaction = LinePlotInteraction{
            .kind = LinePlotInteractionKind::Zoom,
            .zoom_factor = zoom_step,
            .anchor_x = 0.5,
            .anchor_y = 0.5,
        },
    });
    return true;
  }

  if (note == "zoom_out") {
    emitted_commands.push_back(WindowCommand{
        .kind = WindowCommandKind::Zoom,
        .interaction = LinePlotInteraction{
            .kind = LinePlotInteractionKind::Zoom,
            .zoom_factor = 1.0 / zoom_step,
            .anchor_x = 0.5,
            .anchor_y = 0.5,
        },
    });
    return true;
  }

  if (note == "reset_view") {
    emitted_commands.push_back(WindowCommand{
        .kind = WindowCommandKind::ResetView,
        .interaction = LinePlotInteraction{.kind = LinePlotInteractionKind::Reset},
    });
    return true;
  }

  if (note == "pan_left") {
    emitted_commands.push_back(WindowCommand{
        .kind = WindowCommandKind::Pan,
        .interaction = LinePlotInteraction{
            .kind = LinePlotInteractionKind::Pan,
            .dx_ratio = -pan_step,
        },
    });
    return true;
  }

  if (note == "pan_right") {
    emitted_commands.push_back(WindowCommand{
        .kind = WindowCommandKind::Pan,
        .interaction = LinePlotInteraction{
            .kind = LinePlotInteractionKind::Pan,
            .dx_ratio = pan_step,
        },
    });
    return true;
  }

  if (note == "pan_up") {
    emitted_commands.push_back(WindowCommand{
        .kind = WindowCommandKind::Pan,
        .interaction = LinePlotInteraction{
            .kind = LinePlotInteractionKind::Pan,
            .dy_ratio = pan_step,
        },
    });
    return true;
  }

  if (note == "pan_down") {
    emitted_commands.push_back(WindowCommand{
        .kind = WindowCommandKind::Pan,
        .interaction = LinePlotInteraction{
            .kind = LinePlotInteractionKind::Pan,
            .dy_ratio = -pan_step,
        },
    });
    return true;
  }

  return false;
}

[[nodiscard]] bool append_pointer_scroll_commands(const bool table_mode,
                                                  const ShellEvent& event,
                                                  const WindowInputPolicy& policy,
                                                  std::vector<WindowCommand>& emitted_commands) {
  if (table_mode) {
    if (event.delta_y < 0.0) {
      emitted_commands.push_back(WindowCommand{.kind = WindowCommandKind::TableNextPage});
      return true;
    }
    if (event.delta_y > 0.0) {
      emitted_commands.push_back(WindowCommand{.kind = WindowCommandKind::TablePreviousPage});
      return true;
    }
    return false;
  }

  if (event.delta_y == 0.0) {
    return false;
  }

  const double zoom_step = normalized_zoom_factor(policy.scroll_zoom_factor, 1.10);
  const bool zoom_in = policy.invert_scroll_zoom ? (event.delta_y < 0.0) : (event.delta_y > 0.0);
  const double zoom_factor = zoom_in ? zoom_step : (1.0 / zoom_step);
  emitted_commands.push_back(WindowCommand{
      .kind = WindowCommandKind::Zoom,
      .interaction = LinePlotInteraction{
          .kind = LinePlotInteractionKind::Zoom,
          .zoom_factor = zoom_factor,
          .anchor_x = 0.5,
          .anchor_y = 0.5,
      },
  });
  return true;
}

bool process_shell_events(IWindowShell& shell,
                          ViewerRuntime& runtime,
                          LinePlotWindowSessionConfig& config,
                          WindowSessionState& state,
                          std::vector<WindowSessionEvent>& events,
                          bool& should_render,
                          const bool table_mode,
                          std::vector<WindowCommand>& emitted_commands) {
  for (const auto& event : shell.poll_events()) {
    if (event.kind == ShellEventKind::Closed) {
      events.push_back(WindowSessionEvent{
          WindowSessionEventKind::Closed,
          runtime.frame_id(),
          runtime.canvas().size(),
          event.note.empty() ? "shell_closed" : event.note,
      });
      state = WindowSessionState::Closed;
      return false;
    }

    if (event.kind == ShellEventKind::ResizeRequested && event.size.width > 0 &&
        event.size.height > 0) {
      auto runtime_cfg = config.runtime;
      runtime_cfg.canvas_size = event.size;
      if (!runtime.configure(runtime_cfg)) {
        state = WindowSessionState::Failed;
        events.push_back(WindowSessionEvent{
            WindowSessionEventKind::Error,
            runtime.frame_id(),
            runtime.canvas().size(),
            "runtime_reconfigure_failed",
        });
        return false;
      }
      config.runtime = runtime_cfg;
      events.push_back(WindowSessionEvent{
          WindowSessionEventKind::Resized,
          runtime.frame_id(),
          runtime.canvas().size(),
          "session_resized",
      });
      should_render = true;
    }

    if (event.kind == ShellEventKind::ShortcutCommand) {
      if (append_shortcut_commands(table_mode, event.note, config.input, emitted_commands)) {
        should_render = true;
      }
    }

    if (event.kind == ShellEventKind::PointerScroll) {
      if (append_pointer_scroll_commands(table_mode, event, config.input, emitted_commands)) {
        should_render = true;
      }
    }

    if (event.kind == ShellEventKind::Error) {
      events.push_back(WindowSessionEvent{
          WindowSessionEventKind::Warning,
          runtime.frame_id(),
          runtime.canvas().size(),
          event.note.empty() ? "shell_warning" : event.note,
      });
    }
  }
  return true;
}

} // namespace

LinePlotWindowSession::LinePlotWindowSession(LinePlotController controller,
                                             LinePlotWindowSessionConfig config,
                                             std::unique_ptr<IWindowShell> shell)
    : controller_(std::move(controller)),
      config_(std::move(config)),
      runtime_(config_.runtime),
      shell_(std::move(shell)) {
  if (!shell_) {
    shell_ = make_window_shell(config_.shell_policy);
  }
}

bool LinePlotWindowSession::open() {
  if (state_ == WindowSessionState::Failed || state_ == WindowSessionState::Closed || !shell_) {
    push_event_(WindowSessionEventKind::Error, "session_open_invalid_state");
    state_ = WindowSessionState::Failed;
    return false;
  }
  if (state_ == WindowSessionState::Open) {
    return true;
  }

  config_.shell.initial_size = runtime_.canvas().size();
  controller_.set_viewport_size(runtime_.canvas().size());
  if (!shell_->open(config_.shell)) {
    state_ = WindowSessionState::Failed;
    push_event_(WindowSessionEventKind::Error, "shell_open_failed");
    return false;
  }

  state_ = WindowSessionState::Open;
  push_event_(WindowSessionEventKind::Opened, "session_opened");
  return render_and_present_("session_initial_frame");
}

bool LinePlotWindowSession::enqueue(WindowCommand command) {
  if (state_ == WindowSessionState::Failed || state_ == WindowSessionState::Closed) {
    push_event_(WindowSessionEventKind::Warning, "enqueue_ignored_closed_or_failed");
    return false;
  }
  commands_.push_back(std::move(command));
  return true;
}

bool LinePlotWindowSession::tick() {
  if (state_ != WindowSessionState::Open) {
    return state_ == WindowSessionState::Idle;
  }

  bool should_render = dirty_;
  std::vector<WindowCommand> emitted_commands;
  if (!process_shell_events(*shell_,
                            runtime_,
                            config_,
                            state_,
                            events_,
                            should_render,
                            false,
                            emitted_commands)) {
    return state_ != WindowSessionState::Failed;
  }
  controller_.set_viewport_size(runtime_.canvas().size());
  commands_.insert(commands_.end(), std::make_move_iterator(emitted_commands.begin()),
                   std::make_move_iterator(emitted_commands.end()));

  for (const auto& command : commands_) {
    switch (command.kind) {
      case WindowCommandKind::Pan:
      case WindowCommandKind::Zoom:
      case WindowCommandKind::ResetView:
        if (!controller_.apply(command.interaction)) {
          push_event_(WindowSessionEventKind::Warning, "controller_interaction_rejected");
          continue;
        }
        should_render = true;
        break;
      case WindowCommandKind::ReconfigureCanvas:
        if (!runtime_.configure(command.runtime_config)) {
          state_ = WindowSessionState::Failed;
          push_event_(WindowSessionEventKind::Error, "runtime_config_command_failed");
          commands_.clear();
          return false;
        }
        config_.runtime = command.runtime_config;
        controller_.set_viewport_size(runtime_.canvas().size());
        should_render = true;
        break;
      case WindowCommandKind::TableNextPage:
      case WindowCommandKind::TablePreviousPage:
      case WindowCommandKind::TableSetOffset:
        break;
      case WindowCommandKind::Close:
        commands_.clear();
        close();
        return false;
    }
  }
  commands_.clear();

  if (should_render) {
    if (!render_and_present_("session_tick_frame")) {
      return false;
    }
  }
  dirty_ = false;
  return true;
}

bool LinePlotWindowSession::run_steps(const std::size_t max_steps) {
  if (state_ == WindowSessionState::Idle && !open()) {
    return false;
  }
  for (std::size_t step = 0; step < max_steps; ++step) {
    if (!tick()) {
      return state_ != WindowSessionState::Failed;
    }
  }
  return true;
}

void LinePlotWindowSession::close() noexcept {
  if (state_ == WindowSessionState::Closed) {
    return;
  }
  if (shell_) {
    shell_->close();
  }
  state_ = WindowSessionState::Closed;
  push_event_(WindowSessionEventKind::Closed, "session_closed");
}

WindowSessionState LinePlotWindowSession::state() const noexcept {
  return state_;
}

const PixelCanvas& LinePlotWindowSession::canvas() const noexcept {
  return runtime_.canvas();
}

const std::vector<WindowSessionEvent>& LinePlotWindowSession::events() const noexcept {
  return events_;
}

std::uint64_t LinePlotWindowSession::frame_id() const noexcept {
  return runtime_.frame_id();
}

std::size_t LinePlotWindowSession::queued_command_count() const noexcept {
  return commands_.size();
}

const std::string& LinePlotWindowSession::title() const noexcept {
  return config_.shell.title;
}

ShellBackend LinePlotWindowSession::shell_backend() const noexcept {
  return shell_ != nullptr ? shell_->backend() : ShellBackend::None;
}

ShellCapabilities LinePlotWindowSession::shell_capabilities() const noexcept {
  return shell_ != nullptr ? shell_->capabilities() : ShellCapabilities{};
}

bool LinePlotWindowSession::render_and_present_(std::string note) {
  if (state_ != WindowSessionState::Open) {
    push_event_(WindowSessionEventKind::Warning, "render_ignored_not_open");
    return false;
  }

  if (!runtime_.submit(controller_.build_scene())) {
    state_ = WindowSessionState::Failed;
    push_event_(WindowSessionEventKind::Error, "runtime_submit_failed");
    return false;
  }
  if (!shell_->present(runtime_.canvas())) {
    state_ = WindowSessionState::Failed;
    push_event_(WindowSessionEventKind::Error, "shell_present_failed");
    return false;
  }
  push_event_(WindowSessionEventKind::FrameCommitted, std::move(note));
  return true;
}

void LinePlotWindowSession::push_event_(const WindowSessionEventKind kind, std::string note) {
  events_.push_back(WindowSessionEvent{kind, runtime_.frame_id(), runtime_.canvas().size(), std::move(note)});
}

TableWindowSession::TableWindowSession(TableController controller,
                                       LinePlotWindowSessionConfig config,
                                       std::unique_ptr<IWindowShell> shell)
    : controller_(std::move(controller)),
      config_(std::move(config)),
      runtime_(config_.runtime),
      shell_(std::move(shell)) {
  if (!shell_) {
    shell_ = make_window_shell(config_.shell_policy);
  }
}

bool TableWindowSession::open() {
  if (state_ == WindowSessionState::Failed || state_ == WindowSessionState::Closed || !shell_) {
    push_event_(WindowSessionEventKind::Error, "session_open_invalid_state");
    state_ = WindowSessionState::Failed;
    return false;
  }
  if (state_ == WindowSessionState::Open) {
    return true;
  }

  config_.shell.initial_size = runtime_.canvas().size();
  if (!shell_->open(config_.shell)) {
    state_ = WindowSessionState::Failed;
    push_event_(WindowSessionEventKind::Error, "shell_open_failed");
    return false;
  }

  state_ = WindowSessionState::Open;
  push_event_(WindowSessionEventKind::Opened, "session_opened");
  return render_and_present_("session_initial_frame");
}

bool TableWindowSession::tick() {
  if (state_ != WindowSessionState::Open) {
    return state_ == WindowSessionState::Idle;
  }

  bool should_render = dirty_;
  std::vector<WindowCommand> emitted_commands;
  if (!process_shell_events(*shell_,
                            runtime_,
                            config_,
                            state_,
                            events_,
                            should_render,
                            true,
                            emitted_commands)) {
    return state_ != WindowSessionState::Failed;
  }
  commands_.insert(commands_.end(), std::make_move_iterator(emitted_commands.begin()),
                   std::make_move_iterator(emitted_commands.end()));

  for (const auto& command : commands_) {
    switch (command.kind) {
      case WindowCommandKind::TableNextPage:
        (void)controller_.apply(TableInteraction{.kind = TableInteractionKind::NextPage});
        should_render = true;
        break;
      case WindowCommandKind::TablePreviousPage:
        (void)controller_.apply(TableInteraction{.kind = TableInteractionKind::PreviousPage});
        should_render = true;
        break;
      case WindowCommandKind::TableSetOffset:
        (void)controller_.apply(command.table_interaction);
        should_render = true;
        break;
      case WindowCommandKind::ReconfigureCanvas:
        if (!runtime_.configure(command.runtime_config)) {
          state_ = WindowSessionState::Failed;
          push_event_(WindowSessionEventKind::Error, "runtime_config_command_failed");
          commands_.clear();
          return false;
        }
        config_.runtime = command.runtime_config;
        should_render = true;
        break;
      case WindowCommandKind::Close:
        commands_.clear();
        close();
        return false;
      case WindowCommandKind::Pan:
      case WindowCommandKind::Zoom:
      case WindowCommandKind::ResetView:
        break;
    }
  }
  commands_.clear();

  if (should_render) {
    if (!render_and_present_("session_tick_frame")) {
      return false;
    }
  }
  dirty_ = false;
  return true;
}

bool TableWindowSession::enqueue(WindowCommand command) {
  if (state_ == WindowSessionState::Failed || state_ == WindowSessionState::Closed) {
    push_event_(WindowSessionEventKind::Warning, "enqueue_ignored_closed_or_failed");
    return false;
  }
  commands_.push_back(std::move(command));
  return true;
}

void TableWindowSession::close() noexcept {
  if (state_ == WindowSessionState::Closed) {
    return;
  }
  if (shell_) {
    shell_->close();
  }
  state_ = WindowSessionState::Closed;
  push_event_(WindowSessionEventKind::Closed, "session_closed");
}

WindowSessionState TableWindowSession::state() const noexcept {
  return state_;
}

const PixelCanvas& TableWindowSession::canvas() const noexcept {
  return runtime_.canvas();
}

const std::vector<WindowSessionEvent>& TableWindowSession::events() const noexcept {
  return events_;
}

std::uint64_t TableWindowSession::frame_id() const noexcept {
  return runtime_.frame_id();
}

std::size_t TableWindowSession::queued_command_count() const noexcept {
  return commands_.size();
}

const std::string& TableWindowSession::title() const noexcept {
  return config_.shell.title;
}

ShellBackend TableWindowSession::shell_backend() const noexcept {
  return shell_ != nullptr ? shell_->backend() : ShellBackend::None;
}

ShellCapabilities TableWindowSession::shell_capabilities() const noexcept {
  return shell_ != nullptr ? shell_->capabilities() : ShellCapabilities{};
}

bool TableWindowSession::render_and_present_(std::string note) {
  if (state_ != WindowSessionState::Open) {
    push_event_(WindowSessionEventKind::Warning, "render_ignored_not_open");
    return false;
  }

  if (!runtime_.submit(controller_.build_scene())) {
    state_ = WindowSessionState::Failed;
    push_event_(WindowSessionEventKind::Error, "runtime_submit_failed");
    return false;
  }
  if (!shell_->present(runtime_.canvas())) {
    state_ = WindowSessionState::Failed;
    push_event_(WindowSessionEventKind::Error, "shell_present_failed");
    return false;
  }
  push_event_(WindowSessionEventKind::FrameCommitted, std::move(note));
  return true;
}

void TableWindowSession::push_event_(const WindowSessionEventKind kind, std::string note) {
  events_.push_back(WindowSessionEvent{kind, runtime_.frame_id(), runtime_.canvas().size(), std::move(note)});
}

} // namespace pluss::view::window
