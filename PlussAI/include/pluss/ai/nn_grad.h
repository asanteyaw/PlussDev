#pragma once

namespace pluss::codem::series_tape {

bool grad_mode_enabled() noexcept;
void set_grad_mode_enabled(bool enabled) noexcept;

} // namespace pluss::codem::series_tape

namespace pluss::ai::nn {

struct GradMode {
  [[nodiscard]] static bool is_enabled() noexcept {
    return ::pluss::codem::series_tape::grad_mode_enabled();
  }

  static void set_enabled(bool enabled) noexcept {
    ::pluss::codem::series_tape::set_grad_mode_enabled(enabled);
  }
};

class AutoGradMode final {
public:
  explicit AutoGradMode(bool enabled) noexcept : previous_(GradMode::is_enabled()) {
    GradMode::set_enabled(enabled);
  }
  ~AutoGradMode() {
    GradMode::set_enabled(previous_);
  }

  AutoGradMode(const AutoGradMode&) = delete;
  AutoGradMode& operator=(const AutoGradMode&) = delete;
  AutoGradMode(AutoGradMode&&) = delete;
  AutoGradMode& operator=(AutoGradMode&&) = delete;

private:
  bool previous_{true};
};

class NoGradGuard final {
public:
  NoGradGuard() noexcept = default;
  ~NoGradGuard() = default;

  NoGradGuard(const NoGradGuard&) = delete;
  NoGradGuard& operator=(const NoGradGuard&) = delete;
  NoGradGuard(NoGradGuard&&) = delete;
  NoGradGuard& operator=(NoGradGuard&&) = delete;

private:
  AutoGradMode guard_{false};
};

} // namespace pluss::ai::nn
