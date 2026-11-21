// Views for histograms: bin count, what the bars measure, and a density curve over them.

#include "views_internal.h"

namespace pluss::pl::views::internal {

namespace {

[[nodiscard]] bool is_histogram(const Trace& trace) {
  return trace.visible() && !Access::derived(trace) && trace.kind() == TraceKind::Bucket;
}

[[nodiscard]] bool any_histogram(const Studio& studio) {
  for (const Pane& pane : studio.panes()) {
    for (const Trace& trace : pane.traces()) {
      if (is_histogram(trace)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] int automatic_bins(const Vec& values) {
  const Summary s = summarize(values);
  if (s.count < 2 || !(s.max > s.min)) {
    return 10;
  }
  const double n = static_cast<double>(s.count);
  const double iqr = s.q3 - s.q1;
  if (iqr > 0.0) {
    // Freedman-Diaconis: bars as wide as the data's spread and size allow.
    const double width = 2.0 * iqr / std::cbrt(n);
    return std::clamp(static_cast<int>(std::ceil((s.max - s.min) / width)), 5, 80);
  }
  return std::clamp(static_cast<int>(std::ceil(std::log2(n))) + 1, 5, 80);
}

void apply_bins(Studio& studio, std::string_view id, const Choices&) {
  for (Pane& pane : Access::panes(studio)) {
    for (Trace& trace : Access::traces(pane)) {
      auto* data = std::get_if<BucketData>(&Access::payload(trace));
      if (data == nullptr || !is_histogram(trace)) {
        continue;
      }
      data->bins = id == "auto" ? automatic_bins(to_vec(data->values)) : std::stoi(std::string(id.substr(1)));
    }
  }
}

[[nodiscard]] BucketScale scale_of(const Choices& choices) {
  const auto it = choices.find("counts");
  if (it == choices.end()) {
    return BucketScale::Count;
  }
  return it->second == "density"      ? BucketScale::Density
         : it->second == "cumulative" ? BucketScale::Cumulative
         : it->second == "cdf"        ? BucketScale::CumulativeDensity
                                      : BucketScale::Count;
}

void apply_counts(Studio& studio, std::string_view id, const Choices&) {
  const BucketScale scale = id == "density" ? BucketScale::Density : id == "cumulative" ? BucketScale::Cumulative : BucketScale::CumulativeDensity;
  const char* label = scale == BucketScale::Density ? "density" : scale == BucketScale::Cumulative ? "cumulative count" : "cumulative share";
  for (Pane& pane : Access::panes(studio)) {
    bool changed = false;
    for (Trace& trace : Access::traces(pane)) {
      if (auto* data = std::get_if<BucketData>(&Access::payload(trace)); data != nullptr && is_histogram(trace)) {
        data->scale = scale;
        changed = true;
      }
    }
    if (changed) {
      Access::y_axis(pane).label = label;
      pane.y_label_centered(true);  // a long label reads better turned along the axis
    }
  }
}

constexpr double kInvSqrt2Pi = 0.3989422804014327;

void apply_curve(Studio& studio, std::string_view id, const Choices& all) {
  const BucketScale scale = scale_of(all);
  for (Pane& pane : Access::panes(studio)) {
    std::vector<Trace> curves;
    for (const Trace& trace : pane.traces()) {
      const auto* data = std::get_if<BucketData>(&trace.payload());
      if (data == nullptr || !is_histogram(trace)) {
        continue;
      }
      Vec values;
      for (const double v : to_vec(data->values)) {
        if (std::isfinite(v)) {
          values.push_back(v);
        }
      }
      if (values.size() < 3) {
        continue;
      }
      std::sort(values.begin(), values.end());
      double low = data->range.has_value() ? data->range->low : values.front();
      double high = data->range.has_value() ? data->range->high : values.back();
      if (!(high > low)) {
        continue;
      }
      const double bin_width = (high - low) / static_cast<double>(std::max(1, data->bins));
      const double n = static_cast<double>(values.size());
      const Summary s = summarize(values);

      // A thinned sample is as good as the whole for a smooth curve; it keeps the cost of every grid point bounded.
      Vec sample;
      const std::size_t stride = values.size() / 3000 + 1;
      for (std::size_t i = 0; i < values.size(); i += stride) {
        sample.push_back(values[i]);
      }
      const bool normal = id == "normal";
      double bandwidth = 0.0;
      if (!normal) {
        const double spread = s.q3 > s.q1 ? std::min(s.sd, (s.q3 - s.q1) / 1.34) : s.sd;
        const double factor = id == "kde_narrow" ? 0.5 : id == "kde_wide" ? 2.0 : 1.0;
        bandwidth = std::max(1e-12, factor * 0.9 * spread * std::pow(n, -0.2));
      }
      constexpr int kPoints = 200;
      Vec gx(kPoints);
      Vec gy(kPoints);
      for (int k = 0; k < kPoints; ++k) {
        const double x = low + (high - low) * k / (kPoints - 1);
        double pdf = 0.0;
        double cdf = 0.0;
        if (normal) {
          const double u = (x - s.mean) / std::max(s.sd, 1e-12);
          pdf = kInvSqrt2Pi * std::exp(-0.5 * u * u) / std::max(s.sd, 1e-12);
          cdf = 0.5 * std::erfc(-u / std::sqrt(2.0));
        } else {
          for (const double v : sample) {
            const double u = (x - v) / bandwidth;
            pdf += kInvSqrt2Pi * std::exp(-0.5 * u * u);
            cdf += 0.5 * std::erfc(-u / std::sqrt(2.0));
          }
          pdf /= bandwidth * static_cast<double>(sample.size());
          cdf /= static_cast<double>(sample.size());
        }
        gx[static_cast<std::size_t>(k)] = x;
        gy[static_cast<std::size_t>(k)] = scale == BucketScale::Count        ? pdf * n * bin_width
                                          : scale == BucketScale::Density    ? pdf
                                          : scale == BucketScale::Cumulative ? cdf * n
                                                                             : cdf;
      }
      const std::string label = normal ? "normal fit" : "density curve";
      curves.push_back(make_derived(TraceKind::Trend, TrendData{to_series(gx), to_series(gy)}, label,
                                    Rgba::from_u8(214, 80, 48), 2.4F));
    }
    for (auto& curve : curves) {
      Access::traces(pane).push_back(std::move(curve));
    }
  }
}

} // namespace

void register_histogram_views(std::vector<Def>& defs) {
  defs.push_back(Def{
      1100, "bins", "Bins", "Histogram",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (any_histogram(studio)) {
          out = {{"as_drawn", "As drawn"}, {"auto", "Automatic"}, {"b5", "5"},  {"b8", "8"},
                 {"b12", "12"},            {"b20", "20"},          {"b30", "30"}, {"b50", "50"}};
        }
        return out;
      },
      apply_bins});

  defs.push_back(Def{
      1110, "counts", "Bars show", "Histogram",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (any_histogram(studio)) {
          out = {{"count", "Counts"}, {"density", "Density"}, {"cumulative", "Cumulative count"},
                 {"cdf", "Cumulative share"}};
        }
        return out;
      },
      apply_counts});

  defs.push_back(Def{
      1120, "curve", "Curve", "Histogram",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (any_histogram(studio)) {
          out = {{"none", "None"}, {"kde_narrow", "Density, narrow"}, {"kde", "Density"},
                 {"kde_wide", "Density, wide"}, {"normal", "Normal fit"}};
        }
        return out;
      },
      apply_curve});
}

} // namespace pluss::pl::views::internal
