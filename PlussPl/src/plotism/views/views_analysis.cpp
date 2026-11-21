// Views that add analysis to a figure: fit lines, reference lines and bands, value marks, summary statistics, outliers,
// density shading and point styling.

#include "views_internal.h"

namespace pluss::pl::views::internal {

namespace {

const Rgba kAccent = Rgba::from_u8(214, 80, 48);

[[nodiscard]] bool is_cloud(const Trace& trace) { return is_series(trace) && trace.kind() == TraceKind::Cloud; }

[[nodiscard]] bool any_trace(const Studio& studio, const std::function<bool(const Trace&)>& test) {
  for (const Pane& pane : studio.panes()) {
    for (const Trace& trace : pane.traces()) {
      if (test(trace)) {
        return true;
      }
    }
  }
  return false;
}

struct Pairs {
  Vec x;
  Vec y;
};

[[nodiscard]] Pairs finite_pairs(const Trace& trace_const) {
  Trace trace = trace_const;
  const auto ch = channels(trace);
  const Vec x = to_vec(*ch.pos);
  const Vec y = to_vec(*ch.val);
  Pairs out;
  for (std::size_t i = 0; i < x.size() && i < y.size(); ++i) {
    if (std::isfinite(x[i]) && std::isfinite(y[i])) {
      out.x.push_back(x[i]);
      out.y.push_back(y[i]);
    }
  }
  return out;
}

// ---- fit ------------------------------------------------------------------------------------------------------------

void apply_fit(Studio& studio, std::string_view id, const Choices&) {
  const std::int64_t degree = id == "quad" ? 2 : id == "cubic" ? 3 : 1;
  const bool robust = id == "robust";
  const bool lowess = id == "curve";
  const char* what = id == "quad" ? "quadratic" : id == "cubic" ? "cubic" : id == "robust" ? "robust" : id == "curve" ? "smooth" : "linear";
  for (Pane& pane : Access::panes(studio)) {
    const auto traces = series_of(pane);
    std::vector<Trace> fits;
    std::size_t index = 0;
    for (const Trace* trace : traces) {
      if (trace->kind() != TraceKind::Cloud || is_dimmed(*trace)) {
        continue;
      }
      Pairs data = finite_pairs(*trace);
      if (data.x.size() < 4) {
        continue;
      }
      // A smoothing or robust fit looks at every pair for every point it predicts: a thinned sample is as informative.
      if ((lowess || robust) && data.x.size() > 4000) {
        const std::size_t stride = data.x.size() / 4000 + 1;
        Pairs thinned;
        for (std::size_t i = 0; i < data.x.size(); i += stride) {
          thinned.x.push_back(data.x[i]);
          thinned.y.push_back(data.y[i]);
        }
        data = std::move(thinned);
      }
      const auto [low, high] = std::minmax_element(data.x.begin(), data.x.end());
      if (!(*high > *low)) {
        continue;
      }
      Vec query(120);
      for (std::size_t k = 0; k < query.size(); ++k) {
        query[k] = *low + (*high - *low) * static_cast<double>(k) / static_cast<double>(query.size() - 1);
      }
      const Vec pred = flash::detail::regression_predict(data.x, data.y, query, degree, robust, lowess, 0.5);
      const std::string label = trace->label_text().empty() ? std::string(what) + " fit" : trace->label_text() + " (" + what + ")";
      const Rgba color = traces.size() > 1 ? shade_of(drawn_color(*trace), 0.72) : kAccent;
      fits.push_back(make_derived(TraceKind::Trend, TrendData{to_series(query), to_series(pred)}, label, with_alpha(color, 1.0), 2.4F));
      ++index;
    }
    for (auto& fit : fits) {
      Access::traces(pane).push_back(std::move(fit));
    }
  }
}

// ---- guides ---------------------------------------------------------------------------------------------------------

[[nodiscard]] bool guidable(const Trace& trace) {
  return (is_series(trace) && !is_dimmed(trace) && point_count(trace) >= 3) ||
         (trace.visible() && trace.kind() == TraceKind::Bucket && !Access::derived(trace));
}

[[nodiscard]] Vec guide_values(Trace& trace, bool& along_x) {
  if (const auto* bucket = std::get_if<BucketData>(&Access::payload(trace))) {
    along_x = true;
    return to_vec(bucket->values);
  }
  const auto ch = channels(trace);
  along_x = ch.horizontal;
  return to_vec(*ch.val);
}

void add_rule(Pane& pane, bool along_x, double value, std::string label, Rgba color, double width = 1.2) {
  Access::rules(pane).push_back(RuleCue{along_x ? CueAxis::X : CueAxis::Y, value, std::move(label), with_alpha(color, 0.9), width});
}

void add_band(Pane& pane, bool along_x, double low, double high, std::string label, Rgba color) {
  Access::bands(pane).push_back(BandCue{along_x ? CueAxis::X : CueAxis::Y, Range1d{low, high}, std::move(label),
                                        with_alpha(color, 0.55), with_alpha(color, 0.14)});
}

void apply_guides(Studio& studio, std::string_view id, const Choices&) {
  for (Pane& pane : Access::panes(studio)) {
    std::vector<Trace*> targets;
    for (Trace& trace : Access::traces(pane)) {
      if (guidable(trace)) {
        targets.push_back(&trace);
      }
    }
    std::size_t index = 0;
    for (Trace* trace : targets) {
      bool along_x = false;
      const Vec values = guide_values(*trace, along_x);
      const Summary s = summarize(values);
      if (s.count < 3) {
        continue;
      }
      const Rgba color = readable_on(pane, drawn_color(*trace));
      const std::string who = targets.size() > 1 ? series_name(*trace, index) + " " : std::string();
      if (id == "mean") {
        add_rule(pane, along_x, s.mean, who + "mean " + number_text(s.mean), color);
      } else if (id == "median") {
        add_rule(pane, along_x, s.median, who + "median " + number_text(s.median), color);
      } else if (id == "sd1" || id == "sd2") {
        const double k = id == "sd1" ? 1.0 : 2.0;
        add_band(pane, along_x, s.mean - k * s.sd, s.mean + k * s.sd, who + "mean +/- " + number_text(k) + " sd", color);
        add_rule(pane, along_x, s.mean, who + "mean " + number_text(s.mean), color);
      } else if (id == "iqr") {
        add_band(pane, along_x, s.q1, s.q3, who + "middle 50%", color);
        add_rule(pane, along_x, s.median, who + "median " + number_text(s.median), color);
      } else if (id == "range") {
        add_rule(pane, along_x, s.max, who + "max " + number_text(s.max), color, 1.0);
        add_rule(pane, along_x, s.min, who + "min " + number_text(s.min), color, 1.0);
      }
      ++index;
    }
  }
}

// ---- marks ----------------------------------------------------------------------------------------------------------

struct Marker {
  double pos;
  double val;
  bool high;  // the label goes above the point
};

void pin_marker(Pane& pane, const Trace& source, const Marker& marker, bool horizontal, bool right_edge) {
  const std::string text = number_text(marker.val);
  const double width = 5.3 * static_cast<double>(text.size());
  PinCue pin;
  pin.anchor = horizontal ? Vec3d{marker.val, marker.pos, 0.0} : Vec3d{marker.pos, marker.val, 0.0};
  pin.text = text;
  pin.color = readable_on(pane, drawn_color(source));
  if (horizontal) {
    pin.offset_x = marker.val >= 0.0 ? 10.0 : -(width + 12.0);
    pin.offset_y = 0.0;
  } else {
    pin.offset_x = right_edge ? -(width + 10.0) : 8.0;
    pin.offset_y = marker.high ? -14.0 : 17.0;
  }
  Access::pins(pane).push_back(std::move(pin));
}

void apply_marks(Studio& studio, std::string_view id, const Choices&) {
  for (Pane& pane : Access::panes(studio)) {
    for (Trace* trace : series_of(pane)) {
      if (point_count(*trace) < 2 || is_dimmed(*trace)) {
        continue;
      }
      const auto ch = channels(*trace);
      const Vec pos = to_vec(*ch.pos);
      const Vec val = to_vec(*ch.val);
      std::vector<std::size_t> finite;
      for (std::size_t i = 0; i < val.size() && i < pos.size(); ++i) {
        if (std::isfinite(val[i]) && std::isfinite(pos[i])) {
          finite.push_back(i);
        }
      }
      if (finite.size() < 2) {
        continue;
      }
      const auto [lo, hi] = std::minmax_element(finite.begin(), finite.end(),
                                                [&](std::size_t a, std::size_t b) { return val[a] < val[b]; });
      const double span_low = *std::min_element(pos.begin(), pos.end());
      const double span_high = *std::max_element(pos.begin(), pos.end());
      const auto at_right = [&](std::size_t i) { return span_high > span_low && pos[i] > span_high - 0.12 * (span_high - span_low); };
      const auto pin = [&](std::size_t i, bool high) {
        pin_marker(pane, *trace, Marker{pos[i], val[i], high}, ch.horizontal, !ch.horizontal && at_right(i));
      };
      if (id == "extremes") {
        pin(*hi, true);
        if (*lo != *hi) {
          pin(*lo, false);
        }
      } else if (id == "last") {
        pin(finite.back(), val[finite.back()] >= 0.0);
      } else if (id == "ends") {
        pin(finite.front(), val[finite.front()] >= 0.0);
        pin(finite.back(), val[finite.back()] >= 0.0);
      } else if (id == "values") {
        for (const std::size_t i : finite) {
          pin(i, val[i] >= 0.0);
        }
      }
    }
  }
}

// ---- stats ----------------------------------------------------------------------------------------------------------

[[nodiscard]] double correlation(const Pairs& data) {
  const Summary sx = summarize(data.x);
  const Summary sy = summarize(data.y);
  if (data.x.size() < 3 || !(sx.sd > 0.0) || !(sy.sd > 0.0)) {
    return kNaN;
  }
  double acc = 0.0;
  for (std::size_t i = 0; i < data.x.size(); ++i) {
    acc += (data.x[i] - sx.mean) * (data.y[i] - sy.mean);
  }
  return acc / (static_cast<double>(data.x.size() - 1) * sx.sd * sy.sd);
}

void apply_stats(Studio& studio, std::string_view, const Choices&) {
  for (Pane& pane : Access::panes(studio)) {
    std::vector<Trace*> targets;
    for (Trace& trace : Access::traces(pane)) {
      if (guidable(trace)) {
        targets.push_back(&trace);
      }
    }
    if (targets.empty() || targets.size() > 4) {
      continue;
    }
    std::string caption;
    std::size_t index = 0;
    for (Trace* trace : targets) {
      bool along_x = false;
      const Summary s = summarize(guide_values(*trace, along_x));
      if (s.count == 0) {
        continue;
      }
      if (!caption.empty()) {
        caption += "   |   ";
      }
      if (targets.size() > 1) {
        caption += series_name(*trace, index) + ": ";
      }
      caption += "n " + std::to_string(s.count) + "  mean " + number_text(s.mean) + "  sd " + number_text(s.sd) +
                 "  min " + number_text(s.min) + "  median " + number_text(s.median) + "  max " + number_text(s.max);
      if (trace->kind() == TraceKind::Cloud) {
        const double r = correlation(finite_pairs(*trace));
        if (std::isfinite(r)) {
          caption += "  r " + number_text(r);
        }
      }
      ++index;
    }
    pane.caption(caption);
  }
}

// ---- outliers -------------------------------------------------------------------------------------------------------

void apply_outliers(Studio& studio, std::string_view id, const Choices&) {
  for (Pane& pane : Access::panes(studio)) {
    auto& traces = Access::traces(pane);
    std::vector<Trace> extra;
    for (Trace& trace : traces) {
      if (!is_cloud(trace) || point_count(trace) < 10) {
        continue;
      }
      const auto ch = channels(trace);
      const Vec x = to_vec(*ch.pos);
      const Vec y = to_vec(*ch.val);
      const Summary sx = summarize(x);
      const Summary sy = summarize(y);
      std::vector<bool> inlier(x.size(), true);
      std::size_t outside = 0;
      for (std::size_t i = 0; i < x.size(); ++i) {
        const bool far_x = x[i] < sx.q1 - 1.5 * (sx.q3 - sx.q1) || x[i] > sx.q3 + 1.5 * (sx.q3 - sx.q1);
        const bool far_y = y[i] < sy.q1 - 1.5 * (sy.q3 - sy.q1) || y[i] > sy.q3 + 1.5 * (sy.q3 - sy.q1);
        if (far_x || far_y) {
          inlier[i] = false;
          ++outside;
        }
      }
      if (outside == 0 || outside == x.size()) {
        continue;
      }
      if (id == "highlight") {
        Trace flagged = trace;
        std::vector<bool> outlier(inlier.size());
        for (std::size_t i = 0; i < inlier.size(); ++i) {
          outlier[i] = !inlier[i];
        }
        filter_points(flagged, outlier);
        auto payload = Access::payload(flagged);
        if (auto* cloud = std::get_if<CloudData>(&payload); cloud != nullptr && (!cloud->size.has_value() || cloud->size->numel() <= 1)) {
          const double base = cloud->size.has_value() ? to_vec(*cloud->size).front() : StyleOptions{}.default_marker_size;
          cloud->size = to_series(Vec{base * 2.5});  // outliers are few: draw them large enough to find
        }
        flagged = make_derived(TraceKind::Cloud, std::move(payload), "outliers (" + std::to_string(outside) + ")", kAccent, 1.5F);
        extra.push_back(std::move(flagged));
      }
      filter_points(trace, inlier);
    }
    for (auto& flagged : extra) {
      traces.push_back(std::move(flagged));
    }
  }
}

// ---- shade ----------------------------------------------------------------------------------------------------------

[[nodiscard]] bool shadable(const Trace& trace) {
  const auto* cloud = std::get_if<CloudData>(&trace.payload());
  return is_cloud(trace) && point_count(trace) >= 30 && cloud != nullptr && !cloud->color.has_value();
}

void apply_shade(Studio& studio, std::string_view, const Choices&) {
  constexpr int kGrid = 56;
  for (Pane& pane : Access::panes(studio)) {
    for (Trace& trace : Access::traces(pane)) {
      if (!shadable(trace)) {
        continue;
      }
      auto* cloud = std::get_if<CloudData>(&Access::payload(trace));
      const Vec x = to_vec(cloud->x);
      const Vec y = to_vec(cloud->y);
      const Summary sx = summarize(x);
      const Summary sy = summarize(y);
      if (!(sx.max > sx.min) || !(sy.max > sy.min)) {
        continue;
      }
      const auto cell = [&](double v, const Summary& s) {
        return std::clamp(static_cast<int>((v - s.min) / (s.max - s.min) * kGrid), 0, kGrid - 1);
      };
      std::vector<double> counts(static_cast<std::size_t>(kGrid * kGrid), 0.0);
      for (std::size_t i = 0; i < x.size(); ++i) {
        if (std::isfinite(x[i]) && std::isfinite(y[i])) {
          counts[static_cast<std::size_t>(cell(y[i], sy) * kGrid + cell(x[i], sx))] += 1.0;
        }
      }
      // Two passes of a 3x3 box blur make neighbouring cells share their counts: a smooth density, not a mosaic.
      for (int pass = 0; pass < 2; ++pass) {
        std::vector<double> blurred(counts.size(), 0.0);
        for (int r = 0; r < kGrid; ++r) {
          for (int c = 0; c < kGrid; ++c) {
            double sum = 0.0;
            int used = 0;
            for (int dr = -1; dr <= 1; ++dr) {
              for (int dc = -1; dc <= 1; ++dc) {
                const int rr = r + dr;
                const int cc = c + dc;
                if (rr >= 0 && rr < kGrid && cc >= 0 && cc < kGrid) {
                  sum += counts[static_cast<std::size_t>(rr * kGrid + cc)];
                  ++used;
                }
              }
            }
            blurred[static_cast<std::size_t>(r * kGrid + c)] = sum / used;
          }
        }
        counts = std::move(blurred);
      }
      Vec density(x.size(), 0.0);
      for (std::size_t i = 0; i < x.size(); ++i) {
        if (std::isfinite(x[i]) && std::isfinite(y[i])) {
          density[i] = counts[static_cast<std::size_t>(cell(y[i], sy) * kGrid + cell(x[i], sx))];
        }
      }
      // Dense points are drawn last so that they sit on top.
      std::vector<std::size_t> order(x.size());
      std::iota(order.begin(), order.end(), std::size_t{0});
      std::stable_sort(order.begin(), order.end(), [&](auto a, auto b) { return density[a] < density[b]; });
      Vec sorted_x(x.size());
      Vec sorted_y(x.size());
      Vec sorted_d(x.size());
      std::optional<Vec> sizes;
      if (cloud->size.has_value() && static_cast<std::size_t>(cloud->size->numel()) == x.size()) {
        sizes = to_vec(*cloud->size);
      }
      Vec sorted_s;
      for (std::size_t j = 0; j < order.size(); ++j) {
        sorted_x[j] = x[order[j]];
        sorted_y[j] = y[order[j]];
        sorted_d[j] = density[order[j]];
        if (sizes.has_value()) {
          sorted_s.push_back((*sizes)[order[j]]);
        }
      }
      cloud->x = to_series(sorted_x);
      cloud->y = to_series(sorted_y);
      cloud->color = to_series(sorted_d);
      if (sizes.has_value()) {
        cloud->size = to_series(sorted_s);
      }
      trace.palette(PaletteRamp::lagoon());
      trace.stroke(with_alpha(trace.stroke_color(), 1.0));
    }
  }
}

// ---- points ---------------------------------------------------------------------------------------------------------

void apply_dots(Studio& studio, std::string_view id, const Choices&) {
  const double factor = id == "small" ? 0.4 : id == "large" ? 3.5 : 9.0;
  for (Pane& pane : Access::panes(studio)) {
    for (Trace& trace : Access::traces(pane)) {
      auto* cloud = std::get_if<CloudData>(&Access::payload(trace));
      if (cloud == nullptr) {
        continue;
      }
      if (!cloud->size.has_value() || cloud->size->numel() <= 1) {
        const double base = cloud->size.has_value() ? to_vec(*cloud->size).front() : StyleOptions{}.default_marker_size;
        cloud->size = to_series(Vec{base * factor});
      } else {
        Vec sizes = to_vec(*cloud->size);
        for (double& s : sizes) {
          s *= factor;
        }
        cloud->size = to_series(sizes);
      }
    }
  }
}

void apply_opacity(Studio& studio, std::string_view id, const Choices&) {
  const double alpha = id == "faint" ? 0.3 : id == "light" ? 0.55 : 1.0;
  for (Pane& pane : Access::panes(studio)) {
    for (Trace& trace : Access::traces(pane)) {
      if (trace.kind() == TraceKind::Cloud && !Access::derived(trace)) {
        trace.stroke(with_alpha(drawn_color(trace), alpha));
      }
    }
  }
}

} // namespace

void register_analysis_views(std::vector<Def>& defs) {
  defs.push_back(Def{
      1000, "outliers", "Outliers", "Analyze",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (any_trace(studio, [](const Trace& t) { return is_cloud(t) && point_count(t) >= 10; })) {
          out = {{"none", "Not marked"}, {"highlight", "Highlight (beyond 1.5 x IQR)"}, {"remove", "Hide"}};
        }
        return out;
      },
      apply_outliers});

  defs.push_back(Def{
      1010, "shade", "Density", "Analyze",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (any_trace(studio, shadable)) {
          out = {{"off", "Plain points"}, {"on", "Shade by local density"}};
        }
        return out;
      },
      apply_shade});

  defs.push_back(Def{
      1020, "fit", "Fit line", "Analyze",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (any_trace(studio, [](const Trace& t) { return is_cloud(t) && point_count(t) >= 5; })) {
          out = {{"none", "None"},   {"linear", "Linear"}, {"quad", "Quadratic"},
                 {"cubic", "Cubic"}, {"robust", "Robust line"}, {"curve", "Smooth curve"}};
        }
        return out;
      },
      apply_fit});

  defs.push_back(Def{
      960, "guides", "Guides", "Analyze",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (any_trace(studio, guidable)) {
          out = {{"none", "None"},           {"mean", "Mean"},           {"median", "Median"},
                 {"sd1", "Mean +/- 1 SD"},   {"sd2", "Mean +/- 2 SD"},  {"iqr", "Middle 50% (quartiles)"},
                 {"range", "Smallest and largest"}};
        }
        return out;
      },
      apply_guides});

  defs.push_back(Def{
      970, "marks", "Mark", "Analyze",
      [](const Studio& studio) {
        std::vector<Option> out;
        std::int64_t longest = 0;
        bool any = false;
        for (const Pane& pane : studio.panes()) {
          for (const Trace* trace : series_of(pane)) {
            if (point_count(*trace) >= 2) {
              any = true;
              longest = std::max(longest, point_count(*trace));
            }
          }
        }
        if (any) {
          out = {{"none", "None"}, {"extremes", "Highest and lowest"}, {"last", "Last value"}, {"ends", "First and last"}};
          if (longest <= 40) {
            out.push_back({"values", "Every value"});
          }
        }
        return out;
      },
      apply_marks});

  defs.push_back(Def{
      980, "stats", "Statistics", "Analyze",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (any_trace(studio, guidable)) {
          out = {{"none", "None"}, {"summary", "Summary line"}};
        }
        return out;
      },
      apply_stats});

  defs.push_back(Def{
      810, "dots", "Dot size", "Draw",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (any_trace(studio, is_cloud)) {
          out = {{"as_drawn", "As drawn"}, {"small", "Small"}, {"large", "Large"}, {"huge", "Huge"}};
        }
        return out;
      },
      apply_dots});

  defs.push_back(Def{
      820, "opacity", "Opacity", "Draw",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (any_trace(studio, is_cloud)) {
          out = {{"as_drawn", "As drawn"}, {"faint", "Faint"}, {"light", "Light"}, {"solid", "Solid"}};
        }
        return out;
      },
      apply_opacity});
}

} // namespace pluss::pl::views::internal
