#pragma once

#include "pluss/codem/series_options.h"
#include "pluss/detail/data_reader.h"
#include "CodeM/core/SeriesTape.h"

namespace pluss::detail {

struct SeriesBuildState final {
  SeriesAutodiffOptions autodiff{};
};

inline void collect_build(SeriesBuildState& state, const detail::DataReader::Option& opt) {
  if (opt.tracks_slope_set) {
    state.autodiff.tracks_slope = opt.tracks_slope;
  }
  if (opt.retain_sensitivity_set) {
    state.autodiff.retain_sensitivity = opt.retain_sensitivity;
  }
}

inline void collect_build(SeriesBuildState& state, cfg::TracksSlope opt) {
  state.autodiff.tracks_slope = opt.v;
}

inline void collect_build(SeriesBuildState& state, cfg::RetainSensitivity opt) {
  state.autodiff.retain_sensitivity = opt.v;
}

inline void collect_build(SeriesBuildState& state, const SeriesAutodiffOptions& opt) {
  if (opt.tracks_slope.has_value()) {
    state.autodiff.tracks_slope = opt.tracks_slope;
  }
  if (opt.retain_sensitivity.has_value()) {
    state.autodiff.retain_sensitivity = opt.retain_sensitivity;
  }
}

inline void collect_build(SeriesBuildState& state, const SeriesOptions& opt) {
  collect_build(state, opt.autodiff_options());
}

template <class T>
inline void collect_build(SeriesBuildState&, const T&) {}

template <class... Opts>
inline SeriesBuildState build_state(const Opts&... opts) {
  SeriesBuildState state{};
  (collect_build(state, opts), ...);
  return state;
}

inline Series finalize(Series out, const SeriesAutodiffOptions& options) {
  if (!out.defined()) return out;
  if (options.tracks_slope.has_value()) {
    codem::series_tape::set_tracks_slope(out, *options.tracks_slope);
    if (!*options.tracks_slope) {
      codem::series_tape::clear_source_link(out);
      codem::series_tape::clear_sensitivity(out);
    }
  }
  if (options.retain_sensitivity.has_value()) {
    codem::series_tape::set_keep_sensitivity(out, *options.retain_sensitivity);
    if (!*options.retain_sensitivity) {
      codem::series_tape::clear_sensitivity(out);
    }
  }
  return out;
}

} // namespace pluss::detail
