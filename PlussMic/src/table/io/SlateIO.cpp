#include "table/Slate.h"

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

struct SlateView::State {
  bool auto_render_terminal{false};
  mutable bool consumed{false};
  std::string operation{};
};

std::string Slate::to_string(const display::DisplayOptions& options) const {
  return display::render(*this, options);
}

std::string Slate::info_string() const {
  std::int64_t non_null = size();
  if (has_validity_mask()) {
    const auto valid = validity_mask()->to_vec<bool>();
    non_null = static_cast<std::int64_t>(std::count(valid.begin(), valid.end(), true));
  }
  const auto null_count = size() - non_null;
  const auto kind_name = [&]() -> std::string {
    switch (kind()) {
      case SlateKind::Series: return "Series";
      case SlateKind::Note: return "Note";
      case SlateKind::Timeline: return "Timeline";
      case SlateKind::Duration: return "Duration";
    }
    return "Unknown";
  }();

  std::string out;
  out += "Slate Info\n";
  out += fmt::format("Name: {}\n", named() ? name() : "<unnamed>");
  out += fmt::format("Kind: {}\n", kind_name);
  out += fmt::format("Dtype: {}\n", dtype());
  out += fmt::format("Cases: {}\n", size());
  out += fmt::format("Non-Null Cases: {}\n", non_null);
  out += fmt::format("Null Cases: {}", null_count);
  return out;
}

void Slate::info() const {
  info(std::cout);
}

void Slate::info(std::ostream& os) const {
  os << info_string() << '\n';
}

std::ostream& operator<<(std::ostream& os, const Slate& slate) {
  output_render(os, slate);
  return os;
}

void output_render(std::ostream& os, const Slate& slate) {
  display::print(os, slate);
}

SlateView::SlateView() : Slate(), state_(std::make_shared<State>()) {}

SlateView::SlateView(Slate slate,
                       bool auto_render_terminal,
                       std::string operation)
    : Slate(std::move(slate)), state_(std::make_shared<State>()) {
  state_->auto_render_terminal = auto_render_terminal;
  state_->operation = std::move(operation);
}

SlateView::SlateView(const SlateView& other)
    : Slate(other), state_(other.state_) {}

SlateView::SlateView(SlateView&& other) noexcept
    : Slate(std::move(other)), state_(std::move(other.state_)) {}

SlateView& SlateView::operator=(const SlateView& other) {
  if (this == &other) {
    return *this;
  }
  Slate::operator=(other);
  state_ = other.state_;
  return *this;
}

SlateView& SlateView::operator=(SlateView&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  Slate::operator=(std::move(other));
  state_ = std::move(other.state_);
  return *this;
}

SlateView::~SlateView() {
  maybe_autorender_();
}

void SlateView::consume_pending_display_() const {
  if (!state_) {
    return;
  }
  state_->consumed = true;
}

void SlateView::maybe_autorender_() const {
  if (!state_ || !state_->auto_render_terminal || state_->consumed
      || std::uncaught_exceptions() > 0 || !Slate::defined()) {
    return;
  }
  state_->consumed = true;
  display::print(std::cout, static_cast<const Slate&>(*this));
}

void SlateView::show(const display::DisplayOptions& options) const {
  consume_pending_display_();
  auto effective = options;
  if (effective.window_title.empty()) {
    effective.window_title = resolved_window_title_();
  }
  display::show(static_cast<const Slate&>(*this), effective);
}

void SlateView::show(std::ostream& os, const display::DisplayOptions& options) const {
  consume_pending_display_();
  auto effective = options;
  if (effective.window_title.empty()) {
    effective.window_title = resolved_window_title_();
  }
  display::print(os, static_cast<const Slate&>(*this), effective);
}

std::string SlateView::resolved_window_title_() const {
  if (!state_ || state_->operation.empty()) {
    return "Slate";
  }
  return "Slate - " + state_->operation;
}

std::ostream& operator<<(std::ostream& os, const SlateView& slate) {
  output_render(os, slate);
  return os;
}

void output_render(std::ostream& os, const SlateView& slate) {
  slate.consume_pending_display_();
  output_render(os, static_cast<const Slate&>(slate));
}

} // namespace pluss::table
