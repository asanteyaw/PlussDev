#include "table/Shelf.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <utility>


#include "table/display/Display.h"
#include <pluss/tils/kit/output.h>

namespace pluss::table {

struct ShelfView::State {
  bool auto_render_terminal{false};
  mutable bool consumed{false};
  std::string operation{};
};

std::string Shelf::to_string(const display::DisplayOptions& options) const {
  return display::render(*this, options);
}

std::string Shelf::info_string() const {
  std::int64_t non_null = size();
  if (has_mask()) {
    const auto valid = validity_mask()->host<bool>();
    non_null = static_cast<std::int64_t>(std::count(valid.begin(), valid.end(), true));
  }
  const auto null_count = size() - non_null;
  const auto kind_name = [&]() -> std::string {
    switch (kind()) {
      case ShelfKind::Series: return "Series";
      case ShelfKind::Note: return "Note";
      case ShelfKind::Timeline: return "Timeline";
      case ShelfKind::Duration: return "Duration";
    }
    return "Unknown";
  }();

  std::string out;
  out += "Shelf Info\n";
  out += pluss::tils::OutputEngine::format("Name: {}\n", named() ? name() : "<unnamed>");
  out += pluss::tils::OutputEngine::format("Kind: {}\n", kind_name);
  out += pluss::tils::OutputEngine::format("Dtype: {}\n", dtype());
  out += pluss::tils::OutputEngine::format("Cases: {}\n", size());
  out += pluss::tils::OutputEngine::format("Non-Null Cases: {}\n", non_null);
  out += pluss::tils::OutputEngine::format("Null Cases: {}", null_count);
  return out;
}

void Shelf::info() const {
  info(std::cout);
}

void Shelf::info(std::ostream& os) const {
  os << info_string() << '\n';
}

std::ostream& operator<<(std::ostream& os, const Shelf& shelf) {
  output_render(os, shelf);
  return os;
}

void output_render(std::ostream& os, const Shelf& shelf) {
  display::print(os, shelf);
}

ShelfView::ShelfView() : Shelf(), state_(std::make_shared<State>()) {}

ShelfView::ShelfView(Shelf shelf,
                       bool auto_render_terminal,
                       std::string operation)
    : Shelf(std::move(shelf)), state_(std::make_shared<State>()) {
  state_->auto_render_terminal = auto_render_terminal;
  state_->operation = std::move(operation);
}

ShelfView::ShelfView(const ShelfView& other)
    : Shelf(other), state_(other.state_) {}

ShelfView::ShelfView(ShelfView&& other) noexcept
    : Shelf(std::move(other)), state_(std::move(other.state_)) {}

ShelfView& ShelfView::operator=(const ShelfView& other) {
  if (this == &other) {
    return *this;
  }
  Shelf::operator=(other);
  state_ = other.state_;
  return *this;
}

ShelfView& ShelfView::operator=(ShelfView&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  Shelf::operator=(std::move(other));
  state_ = std::move(other.state_);
  return *this;
}

ShelfView::~ShelfView() {
  maybe_autorender_();
}

void ShelfView::consume_pending_display_() const {
  if (!state_) {
    return;
  }
  state_->consumed = true;
}

void ShelfView::maybe_autorender_() const {
  if (!state_ || !state_->auto_render_terminal || state_->consumed
      || std::uncaught_exceptions() > 0 || !Shelf::defined()) {
    return;
  }
  state_->consumed = true;
  display::print(std::cout, static_cast<const Shelf&>(*this));
}

void ShelfView::show(const display::DisplayOptions& options) const {
  consume_pending_display_();
  auto effective = options;
  if (effective.window_title.empty()) {
    effective.window_title = resolved_window_title_();
  }
  display::show(static_cast<const Shelf&>(*this), effective);
}

void ShelfView::show(std::ostream& os, const display::DisplayOptions& options) const {
  consume_pending_display_();
  auto effective = options;
  if (effective.window_title.empty()) {
    effective.window_title = resolved_window_title_();
  }
  display::print(os, static_cast<const Shelf&>(*this), effective);
}

std::string ShelfView::resolved_window_title_() const {
  if (!state_ || state_->operation.empty()) {
    return "Shelf";
  }
  return "Shelf - " + state_->operation;
}

std::ostream& operator<<(std::ostream& os, const ShelfView& shelf) {
  output_render(os, shelf);
  return os;
}

void output_render(std::ostream& os, const ShelfView& shelf) {
  shelf.consume_pending_display_();
  output_render(os, static_cast<const Shelf&>(shelf));
}

} // namespace pluss::table
