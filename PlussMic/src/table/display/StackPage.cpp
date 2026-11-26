#include "table/display/StackPage.h"

#include <algorithm>
#include <atomic>
#include <mutex>

#include <pluss/tils/kit/output.h>

namespace pluss::table::display {

namespace {

// The model a stack's window paints. It forwards to the table of the level shown and changes it when the Level menu
// chooses another, which bumps its revision so the window drops what it kept about the old table (column widths, sort,
// scroll). A level is built when it is first shown and kept for the life of the window: the window's search thread may
// still be reading the table it was shown before.
class SwitchModel final : public OptionsModel {
 public:
  explicit SwitchModel(StackSource source) : source_(std::move(source)) { select(0); }

  void select(std::int64_t level) const {
    level = std::clamp<std::int64_t>(level, 0, std::max<std::int64_t>(0, source_.depth - 1));
    const std::scoped_lock lock(mutex_);
    if (level == level_) {
      return;
    }
    owned_.push_back(source_.table(level));
    current_.store(owned_.back().get(), std::memory_order_release);
    level_ = level;
    revision_.fetch_add(1, std::memory_order_acq_rel);
  }

  void apply(const DisplayOptions& options) const override {
    select(options.level);
    current()->apply(options);
  }

  std::int64_t row_count() const noexcept override { return current()->row_count(); }
  std::int64_t column_count() const noexcept override { return current()->column_count(); }
  std::string row_header() const override { return current()->row_header(); }
  std::string row_label(const std::int64_t row) const override { return current()->row_label(row); }
  std::string column_name(const std::int64_t column) const override { return current()->column_name(column); }
  bool column_numeric(const std::int64_t column) const noexcept override { return current()->column_numeric(column); }
  pluss::view::VirtualTableCell cell(const std::int64_t row, const std::int64_t column) const override {
    return current()->cell(row, column);
  }
  bool row_matches(const std::int64_t row, const std::string_view folded_query) const override {
    return current()->row_matches(row, folded_query);
  }
  bool filtering() const noexcept override { return current()->filtering(); }
  bool row_allowed(const std::int64_t row) const override { return current()->row_allowed(row); }
  pluss::view::RowWindow row_window() const override { return current()->row_window(); }
  bool sortable() const noexcept override { return current()->sortable(); }
  double column_meter(const std::int64_t column) const override { return current()->column_meter(column); }
  std::uint64_t revision() const noexcept override { return revision_.load(std::memory_order_acquire); }

 private:
  const OptionsModel* current() const noexcept { return current_.load(std::memory_order_acquire); }

  StackSource source_;
  mutable std::mutex mutex_{};
  mutable std::vector<std::shared_ptr<OptionsModel>> owned_{};
  mutable std::atomic<const OptionsModel*> current_{nullptr};
  mutable std::int64_t level_{-1};
  mutable std::atomic<std::uint64_t> revision_{0};
};

}  // namespace

ModelPage stack_model_page(StackSource source) {
  ModelPage page;
  page.title = source.title;
  page.levels = source.depth;
  const auto unit = source.unit;
  const auto label = source.label;
  const auto depth = source.depth;
  const auto cards = source.cards;
  const auto title = source.title;
  const bool ragged = source.ragged;
  page.level_label = label;
  page.offer_align = ragged;
  const auto model = std::make_shared<SwitchModel>(std::move(source));
  page.on_options = [model](const DisplayOptions& options) { model->apply(options); };
  page.describe = [unit, label, depth, cards, title](const DisplayOptions& options, std::string& subtitle,
                                                     std::vector<std::pair<std::string, std::string>>& out) {
    const auto level = std::clamp<std::int64_t>(options.level, 0, std::max<std::int64_t>(0, depth - 1));
    const auto name = label ? label(level) : std::string{};
    subtitle = pluss::tils::OutputEngine::format("{} {} of {}{}", unit, level + 1, depth, name.empty() ? std::string{} : " · " + name);
    out = {{"Depth", std::to_string(depth)}, {"Level", pluss::tils::OutputEngine::format("{} of {}", level + 1, depth)}};
    if (cards) {
      auto level_cards = cards(level);
      out.insert(out.end(), level_cards.begin(), level_cards.end());
    }
  };
  page.model = model;
  return page;
}

}  // namespace pluss::table::display
