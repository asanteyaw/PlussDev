#include "table/Thread.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include <fmt/format.h>

#include "table/display/Display.h"

namespace pluss::table {

struct ThreadView::State {
  bool auto_render_terminal{false};
  mutable bool consumed{false};
  std::string operation{};
};

std::string Thread::to_string(const display::DisplayOptions& options) const {
  return display::render(*this, options);
}

std::string Thread::info_string() const {
  std::int64_t non_null = size();
  if (has_validity_mask()) {
    const auto valid = validity_mask()->to_vec<bool>();
    non_null = static_cast<std::int64_t>(std::count(valid.begin(), valid.end(), true));
  }
  const auto null_count = size() - non_null;
  const auto kind_name = [&]() -> std::string {
    switch (kind()) {
      case ThreadKind::Series: return "Series";
      case ThreadKind::Note: return "Note";
      case ThreadKind::Timeline: return "Timeline";
      case ThreadKind::TimeIndex: return "TimeIndex";
      default: return "Unknown";
    }
  }();

  std::string out;
  out += "Thread Info\n";
  out += fmt::format("Name: {}\n", named() ? name() : "<unnamed>");
  out += fmt::format("Kind: {}\n", kind_name);
  out += fmt::format("Dtype: {}\n", dtype());
  out += fmt::format("Cases: {}\n", size());
  out += fmt::format("Non-Null Cases: {}\n", non_null);
  out += fmt::format("Null Cases: {}", null_count);
  return out;
}

void Thread::show(const display::DisplayOptions& options) const {
  display::show(*this, options);
}

void Thread::show(std::ostream& os, const display::DisplayOptions& options) const {
  display::print(os, *this, options);
}

void Thread::info() const {
  info(std::cout);
}

void Thread::info(std::ostream& os) const {
  os << info_string() << '\n';
}

std::ostream& operator<<(std::ostream& os, const Thread& thread) {
  output_render(os, thread);
  return os;
}

void output_render(std::ostream& os, const Thread& thread) {
  thread.show(os);
}

ThreadView::ThreadView() : Thread(), state_(std::make_shared<State>()) {}

ThreadView::ThreadView(Thread thread,
                       bool auto_render_terminal,
                       std::string operation)
    : Thread(std::move(thread)), state_(std::make_shared<State>()) {
  state_->auto_render_terminal = auto_render_terminal;
  state_->operation = std::move(operation);
}

ThreadView::ThreadView(const ThreadView& other)
    : Thread(other), state_(other.state_) {}

ThreadView::ThreadView(ThreadView&& other) noexcept
    : Thread(std::move(other)), state_(std::move(other.state_)) {}

ThreadView& ThreadView::operator=(const ThreadView& other) {
  if (this == &other) {
    return *this;
  }
  Thread::operator=(other);
  state_ = other.state_;
  return *this;
}

ThreadView& ThreadView::operator=(ThreadView&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  Thread::operator=(std::move(other));
  state_ = std::move(other.state_);
  return *this;
}

ThreadView::~ThreadView() {
  maybe_autorender_();
}

void ThreadView::consume_pending_display_() const {
  if (!state_) {
    return;
  }
  state_->consumed = true;
}

void ThreadView::maybe_autorender_() const {
  if (!state_ || !state_->auto_render_terminal || state_->consumed
      || std::uncaught_exceptions() > 0 || !Thread::defined()) {
    return;
  }
  state_->consumed = true;
  display::print(std::cout, static_cast<const Thread&>(*this));
}

void ThreadView::show(const display::DisplayOptions& options) const {
  consume_pending_display_();
  auto effective = options;
  if (effective.window_title.empty()) {
    effective.window_title = resolved_window_title_();
  }
  Thread::show(effective);
}

void ThreadView::show(std::ostream& os, const display::DisplayOptions& options) const {
  consume_pending_display_();
  auto effective = options;
  if (effective.window_title.empty()) {
    effective.window_title = resolved_window_title_();
  }
  Thread::show(os, effective);
}

std::string ThreadView::resolved_window_title_() const {
  if (!state_ || state_->operation.empty()) {
    return "Thread";
  }
  return "Thread - " + state_->operation;
}

std::ostream& operator<<(std::ostream& os, const ThreadView& thread) {
  output_render(os, thread);
  return os;
}

void output_render(std::ostream& os, const ThreadView& thread) {
  thread.consume_pending_display_();
  output_render(os, static_cast<const Thread&>(thread));
}

} // namespace pluss::table
