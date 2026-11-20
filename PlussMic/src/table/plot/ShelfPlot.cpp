#include "table/plot/DataFramePlot.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "CodeM/core/Errors.h"

namespace pluss::table {

namespace {

std::string plot_name_or_(const Shelf& shelf, std::string_view fallback) {
  return shelf.named() ? shelf.name() : std::string(fallback);
}

Shelf unlabeled_plot_shelf_(const Shelf& shelf) {
  auto out = shelf.label().reset();
  if (out.named()) {
    out = out.unname();
  }
  return out;
}

void require_same_size_(const Shelf& left, const Shelf& right, std::string_view where) {
  PLUSS_CHECK(left.size() == right.size(),
              std::string(where) + ": shelf size mismatch");
}

std::optional<Shelf> choose_cases_(const Shelf& first,
                                    const Shelf* second = nullptr,
                                    const Shelf* third = nullptr) {
  if (first.label().has()) {
    return first.label().values();
  }
  if (second != nullptr && second->label().has()) {
    return second->label().values();
  }
  if (third != nullptr && third->label().has()) {
    return third->label().values();
  }
  return std::nullopt;
}

DataFrame single_value_frame_(const Shelf& value, std::string& value_name) {
  value_name = plot_name_or_(value, "value");
  return DataFrame({{value_name, unlabeled_plot_shelf_(value)}}, choose_cases_(value));
}

DataFrame pair_value_frame_(const Shelf& x,
                            const Shelf& value,
                            std::string& x_name,
                            std::string& value_name) {
  require_same_size_(x, value, "Shelf::plot");
  x_name = plot_name_or_(x, "x");
  value_name = plot_name_or_(value, "value");
  return DataFrame({
      {x_name, unlabeled_plot_shelf_(x)},
      {value_name, unlabeled_plot_shelf_(value)},
  }, choose_cases_(value, &x));
}

DataFrame trio_value_frame_(const Shelf& x,
                            const Shelf& value,
                            const Shelf& hue,
                            std::string& x_name,
                            std::string& value_name,
                            std::string& hue_name) {
  require_same_size_(x, value, "Shelf::plot");
  require_same_size_(x, hue, "Shelf::plot");
  x_name = plot_name_or_(x, "x");
  value_name = plot_name_or_(value, "value");
  hue_name = plot_name_or_(hue, "hue");
  return DataFrame({
      {x_name, unlabeled_plot_shelf_(x)},
      {value_name, unlabeled_plot_shelf_(value)},
      {hue_name, unlabeled_plot_shelf_(hue)},
  }, choose_cases_(value, &x, &hue));
}

DataFrame label_value_frame_(const Shelf& labels,
                             const Shelf& value,
                             std::string& label_name,
                             std::string& value_name) {
  require_same_size_(labels, value, "Shelf::plot");
  label_name = plot_name_or_(labels, "label");
  value_name = plot_name_or_(value, "value");
  return DataFrame({
      {label_name, unlabeled_plot_shelf_(labels)},
      {value_name, unlabeled_plot_shelf_(value)},
  }, choose_cases_(value, &labels));
}

DataFrame grouped_value_frame_(const Shelf& groups,
                               const Shelf& value,
                               std::string& group_name,
                               std::string& value_name) {
  require_same_size_(groups, value, "Shelf::plot");
  group_name = plot_name_or_(groups, "group");
  value_name = plot_name_or_(value, "value");
  return DataFrame({
      {group_name, unlabeled_plot_shelf_(groups)},
      {value_name, unlabeled_plot_shelf_(value)},
  }, choose_cases_(value, &groups));
}

} // namespace

ShelfPlotAccessor::ShelfPlotAccessor(Shelf value, DataFramePlotOptions options)
    : base_(std::move(value)), options_(std::move(options)) {}

pluss::pl::Studio ShelfPlotAccessor::line() const {
  return line(LinePlotOptions{});
}

pluss::pl::Studio ShelfPlotAccessor::line(const Shelf& x) const {
  return line(x, LinePlotOptions{});
}

pluss::pl::Studio ShelfPlotAccessor::line(const LinePlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).line({std::string_view(value_name)}, std::nullopt, options);
}

pluss::pl::Studio ShelfPlotAccessor::line(const Shelf& x, const LinePlotOptions& options) const {
  std::string x_name;
  std::string value_name;
  auto frame = pair_value_frame_(x, base_, x_name, value_name);
  return frame.plot(options_).line({std::string_view(value_name)}, std::string_view(x_name), options);
}

pluss::pl::Studio ShelfPlotAccessor::step() const {
  return step(StepPlotOptions{});
}

pluss::pl::Studio ShelfPlotAccessor::step(const Shelf& x) const {
  return step(x, StepPlotOptions{});
}

pluss::pl::Studio ShelfPlotAccessor::step(const StepPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).step({std::string_view(value_name)}, std::nullopt, options);
}

pluss::pl::Studio ShelfPlotAccessor::step(const Shelf& x, const StepPlotOptions& options) const {
  std::string x_name;
  std::string value_name;
  auto frame = pair_value_frame_(x, base_, x_name, value_name);
  return frame.plot(options_).step({std::string_view(value_name)}, std::string_view(x_name), options);
}

pluss::pl::Studio ShelfPlotAccessor::area() const {
  return area(AreaPlotOptions{});
}

pluss::pl::Studio ShelfPlotAccessor::area(const Shelf& x) const {
  return area(x, AreaPlotOptions{});
}

pluss::pl::Studio ShelfPlotAccessor::area(const AreaPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).area({std::string_view(value_name)}, std::nullopt, options);
}

pluss::pl::Studio ShelfPlotAccessor::area(const Shelf& x, const AreaPlotOptions& options) const {
  std::string x_name;
  std::string value_name;
  auto frame = pair_value_frame_(x, base_, x_name, value_name);
  return frame.plot(options_).area({std::string_view(value_name)}, std::string_view(x_name), options);
}

pluss::pl::Studio ShelfPlotAccessor::scatter(const Shelf& x) const {
  return scatter(x, ScatterPlotOptions{});
}

pluss::pl::Studio ShelfPlotAccessor::scatter(const Shelf& x, const ScatterPlotOptions& options) const {
  std::string x_name;
  std::string value_name;
  auto frame = pair_value_frame_(x, base_, x_name, value_name);
  return frame.plot(options_).scatter(x_name, value_name, std::nullopt, options);
}

pluss::pl::Studio ShelfPlotAccessor::scatter(const Shelf& x, const Shelf& hue) const {
  return scatter(x, hue, ScatterPlotOptions{});
}

pluss::pl::Studio ShelfPlotAccessor::scatter(const Shelf& x,
                                              const Shelf& hue,
                                              const ScatterPlotOptions& options) const {
  std::string x_name;
  std::string value_name;
  std::string hue_name;
  auto frame = trio_value_frame_(x, base_, hue, x_name, value_name, hue_name);
  return frame.plot(options_).scatter(x_name, value_name, hue_name, options);
}

pluss::pl::Studio ShelfPlotAccessor::bar() const {
  return bar(BarPlotOptions{});
}

pluss::pl::Studio ShelfPlotAccessor::bar(const Shelf& x) const {
  return bar(x, BarPlotOptions{});
}

pluss::pl::Studio ShelfPlotAccessor::bar(const BarPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  auto accessor = DataFramePlotAccessor(frame, options_);
  return accessor.impl_bar({std::string_view(value_name)}, std::nullopt, options);
}

pluss::pl::Studio ShelfPlotAccessor::bar(const Shelf& x, const BarPlotOptions& options) const {
  std::string x_name;
  std::string value_name;
  auto frame = pair_value_frame_(x, base_, x_name, value_name);
  auto accessor = DataFramePlotAccessor(frame, options_);
  return accessor.impl_bar({std::string_view(value_name)}, std::string_view(x_name), options);
}

pluss::pl::Studio ShelfPlotAccessor::box(const BoxPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).box({std::string_view(value_name)}, options);
}

pluss::pl::Studio ShelfPlotAccessor::violin(const ViolinPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).violin({std::string_view(value_name)}, options);
}

pluss::pl::Studio ShelfPlotAccessor::violin(const Shelf& group) const {
  return violin(group, ViolinPlotOptions{});
}

pluss::pl::Studio ShelfPlotAccessor::violin(const Shelf& group, const ViolinPlotOptions& options) const {
  std::string group_name;
  std::string value_name;
  auto frame = grouped_value_frame_(group, base_, group_name, value_name);
  return frame.plot(options_).violin(value_name, group_name, options);
}

pluss::pl::Studio ShelfPlotAccessor::hist(std::int64_t bins) const {
  HistPlotOptions options;
  options.bins = bins;
  return hist(options);
}

pluss::pl::Studio ShelfPlotAccessor::hist(const HistPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).hist({std::string_view(value_name)}, options);
}

pluss::pl::Studio ShelfPlotAccessor::density(std::int64_t points) const {
  DensityPlotOptions options;
  options.points = points;
  return density(options);
}

pluss::pl::Studio ShelfPlotAccessor::density(const DensityPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).density({std::string_view(value_name)}, options);
}

pluss::pl::Studio ShelfPlotAccessor::pie() const {
  return pie(PiePlotOptions{});
}

pluss::pl::Studio ShelfPlotAccessor::pie(const PiePlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  auto accessor = DataFramePlotAccessor(frame, options_);
  return accessor.impl_pie(value_name, std::nullopt, options);
}

pluss::pl::Studio ShelfPlotAccessor::pie(const Shelf& labels) const {
  return pie(labels, PiePlotOptions{});
}

pluss::pl::Studio ShelfPlotAccessor::pie(const Shelf& labels, const PiePlotOptions& options) const {
  std::string label_name;
  std::string value_name;
  auto frame = label_value_frame_(labels, base_, label_name, value_name);
  auto accessor = DataFramePlotAccessor(frame, options_);
  return accessor.impl_pie(value_name, std::string_view(label_name), options);
}

pluss::pl::Studio ShelfPlotAccessor::lag(const LagPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  auto accessor = DataFramePlotAccessor(frame, options_);
  return accessor.impl_lag(value_name, options);
}

pluss::pl::Studio ShelfPlotAccessor::act(const ActPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  auto accessor = DataFramePlotAccessor(frame, options_);
  return accessor.impl_act(value_name, options);
}

pluss::pl::Studio ShelfPlotAccessor::bootstrap(const BootstrapPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  auto accessor = DataFramePlotAccessor(frame, options_);
  return accessor.impl_bootstrap(value_name, options);
}

pluss::pl::Studio ShelfPlotAccessor::brief(const BriefPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).brief({std::string_view(value_name)}, options);
}

pluss::pl::Studio& ShelfPlotAccessor::brief(pluss::pl::Studio& figure,
                                             const BriefPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).brief(figure, {std::string_view(value_name)}, options);
}

pluss::pl::Studio ShelfPlotAccessor::auto_plot() const {
  return auto_plot(AutoPlotOptions{});
}

pluss::pl::Studio ShelfPlotAccessor::auto_plot(const AutoPlotOptions& options) const {
  (void)options;
  if (base_.label().has() && base_.size() <= 16) {
    return bar();
  }
  return line();
}

ShelfPlotAccessor Shelf::plot() const {
  return ShelfPlotAccessor(*this);
}

ShelfPlotAccessor Shelf::plot(const DataFramePlotOptions& options) const {
  return ShelfPlotAccessor(*this, options);
}

} // namespace pluss::table
