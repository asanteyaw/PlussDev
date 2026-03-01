#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "plot/modules/figure.h"

namespace pluss::plotism {

inline Figure2D line(const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_line(y, std::move(name));
  return fig;
}

inline Figure2D line(const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_line(y, std::move(name));
  return fig;
}

template <typename YInput>
  requires detail::PlotInput<YInput>
inline Figure2D line(const YInput& y, std::string name = "") {
  Figure2D fig;
  fig.add_line(y, std::move(name));
  return fig;
}

inline Figure2D line(const std::vector<double>& x, const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_line(x, y, std::move(name));
  return fig;
}

inline Figure2D line(const pluss::Series& x, const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_line(x, y, std::move(name));
  return fig;
}

template <typename XInput, typename YInput>
  requires detail::PlotInput<XInput> && detail::PlotInput<YInput>
inline Figure2D line(const XInput& x, const YInput& y, std::string name = "") {
  Figure2D fig;
  fig.add_line(x, y, std::move(name));
  return fig;
}

inline Figure2D scatter(const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_scatter(y, std::move(name));
  return fig;
}

inline Figure2D scatter(const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_scatter(y, std::move(name));
  return fig;
}

template <typename YInput>
  requires detail::PlotInput<YInput>
inline Figure2D scatter(const YInput& y, std::string name = "") {
  Figure2D fig;
  fig.add_scatter(y, std::move(name));
  return fig;
}

inline Figure2D scatter(const std::vector<double>& x, const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_scatter(x, y, std::move(name));
  return fig;
}

inline Figure2D scatter(const pluss::Series& x, const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_scatter(x, y, std::move(name));
  return fig;
}

template <typename XInput, typename YInput>
  requires detail::PlotInput<XInput> && detail::PlotInput<YInput>
inline Figure2D scatter(const XInput& x, const YInput& y, std::string name = "") {
  Figure2D fig;
  fig.add_scatter(x, y, std::move(name));
  return fig;
}

inline Figure2D bar(const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_bar(y, std::move(name));
  return fig;
}

inline Figure2D bar(const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_bar(y, std::move(name));
  return fig;
}

template <typename YInput>
  requires detail::PlotInput<YInput>
inline Figure2D bar(const YInput& y, std::string name = "") {
  Figure2D fig;
  fig.add_bar(y, std::move(name));
  return fig;
}

inline Figure2D bar(const std::vector<double>& x, const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_bar(x, y, std::move(name));
  return fig;
}

inline Figure2D bar(const pluss::Series& x, const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_bar(x, y, std::move(name));
  return fig;
}

template <typename XInput, typename YInput>
  requires detail::PlotInput<XInput> && detail::PlotInput<YInput>
inline Figure2D bar(const XInput& x, const YInput& y, std::string name = "") {
  Figure2D fig;
  fig.add_bar(x, y, std::move(name));
  return fig;
}

inline Figure2D hist(const std::vector<double>& values, std::int64_t bins = 20, std::string name = "") {
  Figure2D fig;
  fig.add_hist(values, bins, std::move(name));
  return fig;
}

inline Figure2D hist(const pluss::Series& values, std::int64_t bins = 20, std::string name = "") {
  Figure2D fig;
  fig.add_hist(values, bins, std::move(name));
  return fig;
}

template <typename ValuesInput>
  requires detail::PlotInput<ValuesInput>
inline Figure2D hist(const ValuesInput& values, std::int64_t bins = 20, std::string name = "") {
  Figure2D fig;
  fig.add_hist(values, bins, std::move(name));
  return fig;
}

inline Figure2D step(const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_step(y, std::move(name));
  return fig;
}

inline Figure2D step(const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_step(y, std::move(name));
  return fig;
}

template <typename YInput>
  requires detail::PlotInput<YInput>
inline Figure2D step(const YInput& y, std::string name = "") {
  Figure2D fig;
  fig.add_step(y, std::move(name));
  return fig;
}

inline Figure2D step(const std::vector<double>& x, const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_step(x, y, std::move(name));
  return fig;
}

inline Figure2D step(const pluss::Series& x, const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_step(x, y, std::move(name));
  return fig;
}

template <typename XInput, typename YInput>
  requires detail::PlotInput<XInput> && detail::PlotInput<YInput>
inline Figure2D step(const XInput& x, const YInput& y, std::string name = "") {
  Figure2D fig;
  fig.add_step(x, y, std::move(name));
  return fig;
}

inline Figure2D area(const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_area(y, std::move(name));
  return fig;
}

inline Figure2D area(const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_area(y, std::move(name));
  return fig;
}

template <typename YInput>
  requires detail::PlotInput<YInput>
inline Figure2D area(const YInput& y, std::string name = "") {
  Figure2D fig;
  fig.add_area(y, std::move(name));
  return fig;
}

inline Figure2D area(const std::vector<double>& x, const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_area(x, y, std::move(name));
  return fig;
}

inline Figure2D area(const pluss::Series& x, const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_area(x, y, std::move(name));
  return fig;
}

template <typename XInput, typename YInput>
  requires detail::PlotInput<XInput> && detail::PlotInput<YInput>
inline Figure2D area(const XInput& x, const YInput& y, std::string name = "") {
  Figure2D fig;
  fig.add_area(x, y, std::move(name));
  return fig;
}

inline Figure2D band(const std::vector<double>& x,
                     const std::vector<double>& lower,
                     const std::vector<double>& upper,
                     std::string name = "") {
  Figure2D fig;
  fig.add_band(x, lower, upper, std::move(name));
  return fig;
}

inline Figure2D band(const pluss::Series& x,
                     const pluss::Series& lower,
                     const pluss::Series& upper,
                     std::string name = "") {
  Figure2D fig;
  fig.add_band(x, lower, upper, std::move(name));
  return fig;
}

template <typename XInput, typename LowerInput, typename UpperInput>
  requires detail::PlotInput<XInput> && detail::PlotInput<LowerInput> && detail::PlotInput<UpperInput>
inline Figure2D band(const XInput& x, const LowerInput& lower, const UpperInput& upper, std::string name = "") {
  Figure2D fig;
  fig.add_band(x, lower, upper, std::move(name));
  return fig;
}

inline Figure2D heatmap(const std::vector<std::vector<double>>& values, std::string name = "") {
  Figure2D fig;
  fig.add_heatmap(values, std::move(name));
  return fig;
}

inline Figure2D heatmap(const pluss::Series& values, std::string name = "") {
  Figure2D fig;
  fig.add_heatmap(values, std::move(name));
  return fig;
}

inline Figure2D errorbar(const std::vector<double>& x,
                         const std::vector<double>& y,
                         const std::vector<double>& yerr,
                         std::string name = "") {
  Figure2D fig;
  fig.add_errorbar(x, y, yerr, std::move(name));
  return fig;
}

inline Figure2D errorbar(const pluss::Series& x,
                         const pluss::Series& y,
                         const pluss::Series& yerr,
                         std::string name = "") {
  Figure2D fig;
  fig.add_errorbar(x, y, yerr, std::move(name));
  return fig;
}

template <typename XInput, typename YInput, typename ErrInput>
  requires detail::PlotInput<XInput> && detail::PlotInput<YInput> && detail::PlotInput<ErrInput>
inline Figure2D errorbar(const XInput& x, const YInput& y, const ErrInput& yerr, std::string name = "") {
  Figure2D fig;
  fig.add_errorbar(x, y, yerr, std::move(name));
  return fig;
}

} // namespace pluss::plotism
