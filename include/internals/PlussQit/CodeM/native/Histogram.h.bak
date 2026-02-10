#pragma once

#include <PlussQit/core/Series.h>
#include <PlussQit/native/DispatchStub.h>

namespace pluss::native {

using histogramdd_fn = void(*)(const Series&, const std::optional<Series>&, bool, Series&, const TensorList&);
using histogramdd_linear_fn = void(*)(const Series&, const std::optional<Series>&, bool, Series&, const TensorList&, bool);
using histogram_select_outer_bin_edges_fn = void(*)(const Series& input, const int64_t N, std::vector<double> &leftmost_edges, std::vector<double> &rightmost_edges);

DECLARE_DISPATCH(histogramdd_fn, histogramdd_stub)
DECLARE_DISPATCH(histogramdd_linear_fn, histogramdd_linear_stub)
DECLARE_DISPATCH(histogram_select_outer_bin_edges_fn, histogram_select_outer_bin_edges_stub)

} // namespace pluss::native
