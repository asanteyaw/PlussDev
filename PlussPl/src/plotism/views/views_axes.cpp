// Views about the axes: scale (log and friends), range, and line width.

#include "views_internal.h"

namespace pluss::pl::views::internal {

namespace {

// A pane whose axes the scale and range views may change: lines and points only (a bar's base would be on a log
// axis at zero, and a histogram's counts are not a series of points).
[[nodiscard]] bool scalable(const Pane& pane) {
  if (pane.kind() != PaneKind::Planar) {
    return false;
  }
  bool any = false;
  for (const Trace& trace : pane.traces()) {
    if (!trace.visible()) {
      continue;
    }
    if (Access::derived(trace)) {
      continue;
    }
    if (trace.kind() != TraceKind::Trend && trace.kind() != TraceKind::Cloud) {
      return false;
    }
    any = true;
  }
  return any;
}

struct Positivity {
  bool x{true};
  bool y{true};
  bool any{false};
};

[[nodiscard]] Positivity positivity(const Pane& pane_const) {
  Pane pane = pane_const;
  Positivity out;
  for (Trace& trace : Access::traces(pane)) {
    if (!trace.visible()) {
      continue;
    }
    const auto ch = channels(trace);
    if (!ch) {
      continue;
    }
    out.any = true;
    for (const double v : to_vec(*ch.pos)) {
      if (std::isfinite(v) && v <= 0.0) {
        out.x = false;
      }
    }
    for (const double v : to_vec(*ch.val)) {
      if (std::isfinite(v) && v <= 0.0) {
        out.y = false;
      }
    }
  }
  return out;
}

void apply_scale(Studio& studio, std::string_view id, const Choices&) {
  for (Pane& pane : Access::panes(studio)) {
    if (!scalable(pane)) {
      continue;
    }
    const Positivity ok = positivity(pane);
    if (id == "symlog") {
      pane.scale_y(ScalePolicy::symlog());
      continue;
    }
    const bool want_x = id == "xlog" || id == "loglog";
    const bool want_y = id == "ylog" || id == "loglog";
    if (want_x && ok.x) {
      pane.scale_x(ScalePolicy::log10());
    }
    if (want_y) {
      // A transform earlier in the chain can leave values a log axis cannot draw: use the symmetric log then.
      pane.scale_y(ok.y ? ScalePolicy::log10() : ScalePolicy::symlog());
    }
  }
}

[[nodiscard]] bool has_value_axis(const Pane& pane) {
  const auto series = series_of(pane);
  return pane.kind() == PaneKind::Planar && !series.empty() &&
         std::none_of(series.begin(), series.end(), [](const Trace* t) { return is_horizontal(*t); });
}

void apply_range(Studio& studio, std::string_view id, const Choices&) {
  for (Pane& pane : Access::panes(studio)) {
    if (!has_value_axis(pane)) {
      continue;
    }
    if (id == "tight") {
      pane.y_limit_method(LimitMethod::Tight);
    } else if (id == "padded") {
      pane.y_limit_method(LimitMethod::Padded);
    } else {
      double low = std::numeric_limits<double>::infinity();
      double high = -low;
      for (Trace& trace : Access::traces(pane)) {
        const auto ch = channels(trace);
        if (!ch || !trace.visible()) {
          continue;
        }
        for (const double v : to_vec(*ch.val)) {
          if (std::isfinite(v)) {
            low = std::min(low, v);
            high = std::max(high, v);
          }
        }
      }
      if (!(high >= low)) {
        continue;
      }
      if (id == "zero") {
        const double top = high > 0.0 ? high + 0.05 * (high - std::min(low, 0.0)) : 0.0;
        const double bottom = low < 0.0 ? low - 0.05 * (top - low) : 0.0;
        pane.y_limits({bottom, top > bottom ? top : bottom + 1.0});
      } else {
        const double m = std::max(std::abs(low), std::abs(high)) * 1.05;
        pane.y_limits({-m, m > 0.0 ? m : 1.0});
      }
    }
  }
}

} // namespace

void register_axis_views(std::vector<Def>& defs) {
  defs.push_back(Def{
      850, "width", "Line width", "Draw",
      [](const Studio& studio) {
        std::vector<Option> out;
        for (const Pane& pane : studio.panes()) {
          for (const Trace& trace : pane.traces()) {
            if (trace.visible() && !Access::derived(trace) && (trace.kind() == TraceKind::Trend || trace.kind() == TraceKind::Spike)) {
              return std::vector<Option>{{"as_drawn", "As drawn"}, {"thin", "Thin"}, {"thick", "Thick"}, {"heavy", "Heavy"}};
            }
          }
        }
        return out;
      },
      [](Studio& studio, std::string_view id, const Choices&) {
        const float factor = id == "thin" ? 0.55F : id == "thick" ? 2.0F : 3.2F;
        for (Pane& pane : Access::panes(studio)) {
          for (Trace& trace : Access::traces(pane)) {
            if (trace.kind() == TraceKind::Trend || trace.kind() == TraceKind::Spike) {
              trace.width(std::max(0.5F, trace.stroke_width() * factor));
            }
          }
        }
      }});

  defs.push_back(Def{
      1200, "scale", "Scale", "Axes",
      [](const Studio& studio) {
        std::vector<Option> out;
        bool x_ok = true;
        bool y_ok = true;
        bool any = false;
        for (const Pane& pane : studio.panes()) {
          if (!scalable(pane)) {
            continue;
          }
          const Positivity p = positivity(pane);
          any = true;
          x_ok = x_ok && p.x;
          y_ok = y_ok && p.y;
        }
        if (!any) {
          return out;
        }
        out.push_back({"linear", "Linear"});
        if (y_ok) {
          out.push_back({"ylog", "Log y"});
        }
        if (x_ok) {
          out.push_back({"xlog", "Log x"});
        }
        if (x_ok && y_ok) {
          out.push_back({"loglog", "Log x and y"});
        }
        out.push_back({"symlog", "Symmetric log y"});
        return out;
      },
      apply_scale});

  defs.push_back(Def{
      1210, "range", "Y range", "Axes",
      [](const Studio& studio) {
        std::vector<Option> out;
        for (const Pane& pane : studio.panes()) {
          if (has_value_axis(pane)) {
            return std::vector<Option>{{"auto", "Automatic"}, {"tight", "Tight to the data"}, {"zero", "From zero"},
                                       {"sym", "Centered on zero"}, {"padded", "Padded"}};
          }
        }
        return out;
      },
      apply_range});
}

} // namespace pluss::pl::views::internal
