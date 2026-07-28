#include "table/plot/DataFramePlot.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "CodeM/core/Errors.h"

namespace pluss::table {

namespace {

std::string plot_name_or_(const Thread& thread, std::string_view fallback) {
  return thread.named() ? thread.name() : std::string(fallback);
}

Thread unlabeled_plot_thread_(const Thread& thread) {
  auto out = thread.label().reset();
  if (out.named()) {
    out = out.unname();
  }
  return out;
}

void require_same_size_(const Thread& left, const Thread& right, std::string_view where) {
  PLUSS_CHECK(left.size() == right.size(),
              std::string(where) + ": thread size mismatch");
}

std::optional<Thread> choose_cases_(const Thread& first,
                                    const Thread* second = nullptr,
                                    const Thread* third = nullptr) {
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

DataFrame single_value_frame_(const Thread& value, std::string& value_name) {
  value_name = plot_name_or_(value, "value");
  return DataFrame({{value_name, unlabeled_plot_thread_(value)}}, choose_cases_(value));
}

DataFrame pair_value_frame_(const Thread& x,
                            const Thread& value,
                            std::string& x_name,
                            std::string& value_name) {
  require_same_size_(x, value, "Thread::plot");
  x_name = plot_name_or_(x, "x");
  value_name = plot_name_or_(value, "value");
  return DataFrame({
      {x_name, unlabeled_plot_thread_(x)},
      {value_name, unlabeled_plot_thread_(value)},
  }, choose_cases_(value, &x));
}

DataFrame trio_value_frame_(const Thread& x,
                            const Thread& value,
                            const Thread& hue,
                            std::string& x_name,
                            std::string& value_name,
                            std::string& hue_name) {
  require_same_size_(x, value, "Thread::plot");
  require_same_size_(x, hue, "Thread::plot");
  x_name = plot_name_or_(x, "x");
  value_name = plot_name_or_(value, "value");
  hue_name = plot_name_or_(hue, "hue");
  return DataFrame({
      {x_name, unlabeled_plot_thread_(x)},
      {value_name, unlabeled_plot_thread_(value)},
      {hue_name, unlabeled_plot_thread_(hue)},
  }, choose_cases_(value, &x, &hue));
}

DataFrame label_value_frame_(const Thread& labels,
                             const Thread& value,
                             std::string& label_name,
                             std::string& value_name) {
  require_same_size_(labels, value, "Thread::plot");
  label_name = plot_name_or_(labels, "label");
  value_name = plot_name_or_(value, "value");
  return DataFrame({
      {label_name, unlabeled_plot_thread_(labels)},
      {value_name, unlabeled_plot_thread_(value)},
  }, choose_cases_(value, &labels));
}

DataFrame grouped_value_frame_(const Thread& groups,
                               const Thread& value,
                               std::string& group_name,
                               std::string& value_name) {
  require_same_size_(groups, value, "Thread::plot");
  group_name = plot_name_or_(groups, "group");
  value_name = plot_name_or_(value, "value");
  return DataFrame({
      {group_name, unlabeled_plot_thread_(groups)},
      {value_name, unlabeled_plot_thread_(value)},
  }, choose_cases_(value, &groups));
}

} // namespace

ThreadPlotAccessor::ThreadPlotAccessor(Thread value, DataFramePlotOptions options)
    : base_(std::move(value)), options_(std::move(options)) {}

pluss::pl::Studio ThreadPlotAccessor::line() const {
  return line(LinePlotOptions{});
}

pluss::pl::Studio ThreadPlotAccessor::line(const Thread& x) const {
  return line(x, LinePlotOptions{});
}

pluss::pl::Studio ThreadPlotAccessor::line(const LinePlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).line({std::string_view(value_name)}, std::nullopt, options);
}

pluss::pl::Studio ThreadPlotAccessor::line(const Thread& x, const LinePlotOptions& options) const {
  std::string x_name;
  std::string value_name;
  auto frame = pair_value_frame_(x, base_, x_name, value_name);
  return frame.plot(options_).line({std::string_view(value_name)}, std::string_view(x_name), options);
}

pluss::pl::Studio ThreadPlotAccessor::step() const {
  return step(StepPlotOptions{});
}

pluss::pl::Studio ThreadPlotAccessor::step(const Thread& x) const {
  return step(x, StepPlotOptions{});
}

pluss::pl::Studio ThreadPlotAccessor::step(const StepPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).step({std::string_view(value_name)}, std::nullopt, options);
}

pluss::pl::Studio ThreadPlotAccessor::step(const Thread& x, const StepPlotOptions& options) const {
  std::string x_name;
  std::string value_name;
  auto frame = pair_value_frame_(x, base_, x_name, value_name);
  return frame.plot(options_).step({std::string_view(value_name)}, std::string_view(x_name), options);
}

pluss::pl::Studio ThreadPlotAccessor::area() const {
  return area(AreaPlotOptions{});
}

pluss::pl::Studio ThreadPlotAccessor::area(const Thread& x) const {
  return area(x, AreaPlotOptions{});
}

pluss::pl::Studio ThreadPlotAccessor::area(const AreaPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).area({std::string_view(value_name)}, std::nullopt, options);
}

pluss::pl::Studio ThreadPlotAccessor::area(const Thread& x, const AreaPlotOptions& options) const {
  std::string x_name;
  std::string value_name;
  auto frame = pair_value_frame_(x, base_, x_name, value_name);
  return frame.plot(options_).area({std::string_view(value_name)}, std::string_view(x_name), options);
}

pluss::pl::Studio ThreadPlotAccessor::scatter(const Thread& x) const {
  return scatter(x, ScatterPlotOptions{});
}

pluss::pl::Studio ThreadPlotAccessor::scatter(const Thread& x, const ScatterPlotOptions& options) const {
  std::string x_name;
  std::string value_name;
  auto frame = pair_value_frame_(x, base_, x_name, value_name);
  return frame.plot(options_).scatter(x_name, value_name, std::nullopt, options);
}

pluss::pl::Studio ThreadPlotAccessor::scatter(const Thread& x, const Thread& hue) const {
  return scatter(x, hue, ScatterPlotOptions{});
}

pluss::pl::Studio ThreadPlotAccessor::scatter(const Thread& x,
                                              const Thread& hue,
                                              const ScatterPlotOptions& options) const {
  std::string x_name;
  std::string value_name;
  std::string hue_name;
  auto frame = trio_value_frame_(x, base_, hue, x_name, value_name, hue_name);
  return frame.plot(options_).scatter(x_name, value_name, hue_name, options);
}

pluss::pl::Studio ThreadPlotAccessor::bar() const {
  return bar(BarPlotOptions{});
}

pluss::pl::Studio ThreadPlotAccessor::bar(const Thread& x) const {
  return bar(x, BarPlotOptions{});
}

pluss::pl::Studio ThreadPlotAccessor::bar(const BarPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  auto accessor = DataFramePlotAccessor(frame, options_);
  return accessor.impl_bar({std::string_view(value_name)}, std::nullopt, options);
}

pluss::pl::Studio ThreadPlotAccessor::bar(const Thread& x, const BarPlotOptions& options) const {
  std::string x_name;
  std::string value_name;
  auto frame = pair_value_frame_(x, base_, x_name, value_name);
  auto accessor = DataFramePlotAccessor(frame, options_);
  return accessor.impl_bar({std::string_view(value_name)}, std::string_view(x_name), options);
}

pluss::pl::Studio ThreadPlotAccessor::box(const BoxPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).box({std::string_view(value_name)}, options);
}

pluss::pl::Studio ThreadPlotAccessor::violin(const ViolinPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).violin({std::string_view(value_name)}, options);
}

pluss::pl::Studio ThreadPlotAccessor::violin(const Thread& group) const {
  return violin(group, ViolinPlotOptions{});
}

pluss::pl::Studio ThreadPlotAccessor::violin(const Thread& group, const ViolinPlotOptions& options) const {
  std::string group_name;
  std::string value_name;
  auto frame = grouped_value_frame_(group, base_, group_name, value_name);
  return frame.plot(options_).violin(value_name, group_name, options);
}

pluss::pl::Studio ThreadPlotAccessor::hist(std::int64_t bins) const {
  HistPlotOptions options;
  options.bins = bins;
  return hist(options);
}

pluss::pl::Studio ThreadPlotAccessor::hist(const HistPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).hist({std::string_view(value_name)}, options);
}

pluss::pl::Studio ThreadPlotAccessor::density(std::int64_t points) const {
  DensityPlotOptions options;
  options.points = points;
  return density(options);
}

pluss::pl::Studio ThreadPlotAccessor::density(const DensityPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).density({std::string_view(value_name)}, options);
}

pluss::pl::Studio ThreadPlotAccessor::pie() const {
  return pie(PiePlotOptions{});
}

pluss::pl::Studio ThreadPlotAccessor::pie(const PiePlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  auto accessor = DataFramePlotAccessor(frame, options_);
  return accessor.impl_pie(value_name, std::nullopt, options);
}

pluss::pl::Studio ThreadPlotAccessor::pie(const Thread& labels) const {
  return pie(labels, PiePlotOptions{});
}

pluss::pl::Studio ThreadPlotAccessor::pie(const Thread& labels, const PiePlotOptions& options) const {
  std::string label_name;
  std::string value_name;
  auto frame = label_value_frame_(labels, base_, label_name, value_name);
  auto accessor = DataFramePlotAccessor(frame, options_);
  return accessor.impl_pie(value_name, std::string_view(label_name), options);
}

pluss::pl::Studio ThreadPlotAccessor::lag(const LagPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  auto accessor = DataFramePlotAccessor(frame, options_);
  return accessor.impl_lag(value_name, options);
}

pluss::pl::Studio ThreadPlotAccessor::act(const ActPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  auto accessor = DataFramePlotAccessor(frame, options_);
  return accessor.impl_act(value_name, options);
}

pluss::pl::Studio ThreadPlotAccessor::bootstrap(const BootstrapPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  auto accessor = DataFramePlotAccessor(frame, options_);
  return accessor.impl_bootstrap(value_name, options);
}

pluss::pl::Studio ThreadPlotAccessor::brief(const BriefPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).brief({std::string_view(value_name)}, options);
}

pluss::pl::Studio& ThreadPlotAccessor::brief(pluss::pl::Studio& figure,
                                             const BriefPlotOptions& options) const {
  std::string value_name;
  auto frame = single_value_frame_(base_, value_name);
  return frame.plot(options_).brief(figure, {std::string_view(value_name)}, options);
}

pluss::pl::Studio ThreadPlotAccessor::auto_plot() const {
  return auto_plot(AutoPlotOptions{});
}

pluss::pl::Studio ThreadPlotAccessor::auto_plot(const AutoPlotOptions& options) const {
  (void)options;
  if (base_.label().has() && base_.size() <= 16) {
    return bar();
  }
  return line();
}

ThreadPlotAccessor Thread::plot() const {
  return ThreadPlotAccessor(*this);
}

ThreadPlotAccessor Thread::plot(const DataFramePlotOptions& options) const {
  return ThreadPlotAccessor(*this, options);
}

} // namespace pluss::table
