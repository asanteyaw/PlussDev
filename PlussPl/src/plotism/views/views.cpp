#include "pluss/pl/views.h"

#include "views_internal.h"

namespace pluss::pl::views {

namespace {

const std::vector<internal::Def>& registry() {
  static const std::vector<internal::Def> defs = [] {
    std::vector<internal::Def> list;
    internal::register_look_views(list);
    internal::register_series_views(list);
    internal::register_data_views(list);
    internal::register_analysis_views(list);
    internal::register_histogram_views(list);
    internal::register_axis_views(list);
    std::stable_sort(list.begin(), list.end(),
                     [](const internal::Def& a, const internal::Def& b) { return a.stage < b.stage; });
    return list;
  }();
  return defs;
}

// The order the window lists sections in; a group's own place inside its section follows its stage.
int section_rank(const std::string& section) {
  static const std::vector<std::string> order{"", "Series", "Data", "Histogram", "Draw", "Analyze", "Axes"};
  const auto it = std::find(order.begin(), order.end(), section);
  return static_cast<int>(it - order.begin());
}

} // namespace

std::vector<Group> groups(const Studio& figure) {
  std::vector<std::pair<int, Group>> found;
  int order = 0;
  for (const auto& def : registry()) {
    auto options = def.options(figure);
    if (options.size() >= 2) {
      found.push_back({order, Group{def.id, def.title, def.section, std::move(options)}});
    }
    ++order;
  }
  std::stable_sort(found.begin(), found.end(), [](const auto& a, const auto& b) {
    return section_rank(a.second.section) < section_rank(b.second.section);
  });
  std::vector<Group> out;
  out.reserve(found.size());
  for (auto& entry : found) {
    out.push_back(std::move(entry.second));
  }
  return out;
}

Studio apply(const Studio& figure, const Choices& choices) {
  Studio copy = figure;
  // A ribbon's values sit on top of its base: fold the base in, so that every view sees the values as drawn.
  for (Pane& pane : internal::Access::panes(copy)) {
    for (Trace& trace : internal::Access::traces(pane)) {
      auto* ribbon = std::get_if<RibbonData>(&internal::Access::payload(trace));
      if (ribbon != nullptr && ribbon->base != 0.0 && ribbon->y.dim() <= 1) {
        ribbon->y = ribbon->y.add(ribbon->base);
        ribbon->base = 0.0;
      }
    }
  }
  for (const auto& def : registry()) {
    const auto chosen = choices.find(def.id);
    if (chosen == choices.end()) {
      continue;
    }
    // Only an option the group really offers for this figure counts, and its first (default) option is a no-op.
    const auto options = def.options(figure);
    if (options.size() < 2 || chosen->second == options.front().id) {
      continue;
    }
    const bool offered = std::any_of(options.begin(), options.end(),
                                     [&](const Option& option) { return option.id == chosen->second; });
    if (offered) {
      def.apply(copy, chosen->second, choices);
    }
  }
  // A fit, an outlier overlay or a density curve is named in the legend, so the legend has to be there to name it.
  const auto legend = choices.find("legend");
  if (legend == choices.end() || legend->second != "off") {
    for (Pane& pane : internal::Access::panes(copy)) {
      const bool named = std::any_of(pane.traces().begin(), pane.traces().end(), [](const Trace& trace) {
        return internal::Access::derived(trace) && !trace.label_text().empty();
      });
      if (named && pane.legend_place() == KeyPlace::None && copy.legend_dock() == KeyDock::None) {
        pane.legend(KeyPlace::Auto);
      }
    }
  }
  return copy;
}

} // namespace pluss::pl::views
