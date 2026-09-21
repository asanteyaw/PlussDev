#include "CodeM/native/cpu/GroupedReductions.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/ScalarType.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::codem::native::cpu {
namespace {

void require_grouped_args_(
    const Series& values,
    const Series& group_ids,
    std::int64_t group_count,
    const char* where) {
  PLUSS_CHECK(values.defined(), std::string(where) + ": values Series is undefined");
  PLUSS_CHECK(group_ids.defined(), std::string(where) + ": group_ids Series is undefined");
  PLUSS_CHECK(values.platform() == Platform::CPU,
              std::string(where) + ": values must be on CPU");
  PLUSS_CHECK(group_ids.platform() == Platform::CPU,
              std::string(where) + ": group_ids must be on CPU");
  PLUSS_CHECK(values.dim() == 1, std::string(where) + ": values must be 1D");
  PLUSS_CHECK(group_ids.dim() == 1, std::string(where) + ": group_ids must be 1D");
  PLUSS_CHECK(group_ids.scalar_type() == ScalarType::Int64,
              std::string(where) + ": group_ids must be int64");
  PLUSS_CHECK(group_ids.numel() == values.numel(),
              std::string(where) + ": group_ids length must match values length");
  PLUSS_CHECK(group_count >= 0, std::string(where) + ": group_count must be non-negative");
}

double load_value_as_double_(const Float16 value) {
  return static_cast<double>(float16_to_float(value));
}

double load_value_as_double_(const BFloat16 value) {
  return static_cast<double>(bfloat16_to_float(value));
}

double load_value_as_double_(const Float8E4M3FN value) {
  return static_cast<double>(float8e4m3fn_to_float(value));
}

double load_value_as_double_(const Float8E5M2 value) {
  return static_cast<double>(float8e5m2_to_float(value));
}

template <class T>
double load_value_as_double_(const T value) {
  if constexpr (std::is_same_v<T, bool>) {
    return value ? 1.0 : 0.0;
  } else {
    return static_cast<double>(value);
  }
}

template <class T>
void compact_by_group_(
    const Series& values,
    const Series& group_ids,
    std::int64_t group_count,
    std::vector<double>& grouped_values,
    std::vector<std::int64_t>& grouped_rows,
    std::vector<std::int64_t>& offsets,
    const char* where) {
  const auto n = values.numel();
  const auto* value_ptr = reinterpret_cast<const T*>(values.raw_data());
  const auto* id_ptr = reinterpret_cast<const std::int64_t*>(group_ids.raw_data());

  std::vector<std::int64_t> counts(static_cast<std::size_t>(group_count), 0);
  for (std::int64_t row = 0; row < n; ++row) {
    const auto group = id_ptr[row];
    PLUSS_CHECK(group >= 0 && group < group_count,
                std::string(where) + ": group id out of range");
    ++counts[static_cast<std::size_t>(group)];
  }

  offsets.assign(static_cast<std::size_t>(group_count) + 1, 0);
  for (std::int64_t group = 0; group < group_count; ++group) {
    offsets[static_cast<std::size_t>(group + 1)] =
        offsets[static_cast<std::size_t>(group)] +
        counts[static_cast<std::size_t>(group)];
  }

  grouped_values.assign(static_cast<std::size_t>(n), 0.0);
  grouped_rows.assign(static_cast<std::size_t>(n), 0);
  auto cursor = offsets;
  for (std::int64_t row = 0; row < n; ++row) {
    const auto group = id_ptr[row];
    const auto write =
        cursor[static_cast<std::size_t>(group)]++;
    grouped_values[static_cast<std::size_t>(write)] =
        load_value_as_double_(value_ptr[row]);
    grouped_rows[static_cast<std::size_t>(write)] = row;
  }
}

void write_float64_output_(Series& out, const std::vector<double>& values) {
  if (!values.empty()) {
    std::memcpy(out.raw_data(), values.data(), values.size() * sizeof(double));
  }
}

bool grouped_mode_value_less_(double lhs, double rhs) noexcept {
  const bool lhs_nan = std::isnan(lhs);
  const bool rhs_nan = std::isnan(rhs);
  if (lhs_nan || rhs_nan) {
    return !lhs_nan && rhs_nan;
  }
  return lhs < rhs;
}

bool grouped_mode_value_equal_(double lhs, double rhs) noexcept {
  if (std::isnan(lhs) && std::isnan(rhs)) {
    return true;
  }
  return lhs == rhs;
}

template <class T>
Series grouped_median_impl_(
    const Series& values_raw,
    const Series& group_ids_raw,
    std::int64_t group_count) {
  const auto values = values_raw.contiguous();
  const auto group_ids = group_ids_raw.contiguous();

  std::vector<double> grouped_values;
  std::vector<std::int64_t> grouped_rows;
  std::vector<std::int64_t> offsets;
  compact_by_group_<T>(
      values,
      group_ids,
      group_count,
      grouped_values,
      grouped_rows,
      offsets,
      "native::cpu::grouped_median");

  std::vector<double> medians(static_cast<std::size_t>(group_count),
                              std::numeric_limits<double>::quiet_NaN());
  for (std::int64_t group = 0; group < group_count; ++group) {
    const auto begin_index = offsets[static_cast<std::size_t>(group)];
    const auto stop_index = offsets[static_cast<std::size_t>(group + 1)];
    const auto n = stop_index - begin_index;
    if (n <= 0) {
      continue;
    }

    auto begin = grouped_values.begin() + begin_index;
    auto stop = grouped_values.begin() + stop_index;
    auto upper_it = begin + n / 2;
    std::nth_element(begin, upper_it, stop);
    const auto upper = *upper_it;
    if (n % 2 != 0) {
      medians[static_cast<std::size_t>(group)] = upper;
      continue;
    }

    auto lower_it = begin + (n / 2 - 1);
    std::nth_element(begin, lower_it, stop);
    medians[static_cast<std::size_t>(group)] = (*lower_it + upper) / 2.0;
  }

  auto out = Series::empty({group_count}, ScalarType::Float64, Platform::CPU);
  write_float64_output_(out, medians);
  return out;
}

// Same selection as grouped_median_impl_ (same nth_element target ranks),
// but keeps the lower/upper order statistics separate instead of averaging
// them into one value -- read-only, for Series::grouped_median's reverse-mode
// node to route gradient to the specific elements each bound came from.
// Independent recomputation, not a refactor of grouped_median_impl_: the
// forward path above is untouched, zero risk of it regressing.
template <class T>
void grouped_median_bounds_impl_(
    const Series& values_raw,
    const Series& group_ids_raw,
    std::int64_t group_count,
    std::vector<double>& lower_out,
    std::vector<double>& upper_out) {
  const auto values = values_raw.contiguous();
  const auto group_ids = group_ids_raw.contiguous();

  std::vector<double> grouped_values;
  std::vector<std::int64_t> grouped_rows;
  std::vector<std::int64_t> offsets;
  compact_by_group_<T>(
      values,
      group_ids,
      group_count,
      grouped_values,
      grouped_rows,
      offsets,
      "native::cpu::grouped_median_bounds");

  lower_out.assign(static_cast<std::size_t>(group_count), std::numeric_limits<double>::quiet_NaN());
  upper_out.assign(static_cast<std::size_t>(group_count), std::numeric_limits<double>::quiet_NaN());
  for (std::int64_t group = 0; group < group_count; ++group) {
    const auto begin_index = offsets[static_cast<std::size_t>(group)];
    const auto stop_index = offsets[static_cast<std::size_t>(group + 1)];
    const auto n = stop_index - begin_index;
    if (n <= 0) {
      continue;
    }

    auto begin = grouped_values.begin() + begin_index;
    auto stop = grouped_values.begin() + stop_index;
    auto upper_it = begin + n / 2;
    std::nth_element(begin, upper_it, stop);
    const auto upper = *upper_it;
    if (n % 2 != 0) {
      lower_out[static_cast<std::size_t>(group)] = upper;
      upper_out[static_cast<std::size_t>(group)] = upper;
      continue;
    }

    auto lower_it = begin + (n / 2 - 1);
    std::nth_element(begin, lower_it, stop);
    lower_out[static_cast<std::size_t>(group)] = *lower_it;
    upper_out[static_cast<std::size_t>(group)] = upper;
  }
}

template <class T>
Series grouped_mode_impl_(
    const Series& values_raw,
    const Series& group_ids_raw,
    std::int64_t group_count) {
  const auto values = values_raw.contiguous();
  const auto group_ids = group_ids_raw.contiguous();

  std::vector<double> grouped_values;
  std::vector<std::int64_t> grouped_rows;
  std::vector<std::int64_t> offsets;
  compact_by_group_<T>(
      values,
      group_ids,
      group_count,
      grouped_values,
      grouped_rows,
      offsets,
      "native::cpu::grouped_mode");

  std::vector<double> modes(static_cast<std::size_t>(group_count),
                            std::numeric_limits<double>::quiet_NaN());
  std::vector<std::pair<double, std::int64_t>> group_values;
  for (std::int64_t group = 0; group < group_count; ++group) {
    const auto begin_index = offsets[static_cast<std::size_t>(group)];
    const auto stop_index = offsets[static_cast<std::size_t>(group + 1)];
    const auto n = stop_index - begin_index;
    if (n <= 0) {
      continue;
    }

    group_values.clear();
    group_values.reserve(static_cast<std::size_t>(n));
    for (std::int64_t pos = begin_index; pos < stop_index; ++pos) {
      group_values.emplace_back(
          grouped_values[static_cast<std::size_t>(pos)],
          grouped_rows[static_cast<std::size_t>(pos)]);
    }
    std::sort(
        group_values.begin(),
        group_values.end(),
        [](const auto& lhs, const auto& rhs) {
          if (grouped_mode_value_equal_(lhs.first, rhs.first)) {
            return lhs.second < rhs.second;
          }
          return grouped_mode_value_less_(lhs.first, rhs.first);
        });

    std::int64_t best_count = 0;
    std::int64_t best_row = -1;
    double best_value = std::numeric_limits<double>::quiet_NaN();

    std::int64_t run_count = 0;
    std::int64_t run_first_row = -1;
    double run_value = std::numeric_limits<double>::quiet_NaN();
    bool has_run = false;

    auto finish_run = [&]() {
      if (!has_run) {
        return;
      }
      if (run_count > best_count ||
          (run_count == best_count &&
           (best_row < 0 || run_first_row < best_row))) {
        best_count = run_count;
        best_row = run_first_row;
        best_value = run_value;
      }
    };

    for (const auto& [value, row] : group_values) {
      if (!has_run || !grouped_mode_value_equal_(value, run_value)) {
        finish_run();
        has_run = true;
        run_value = value;
        run_count = 1;
        run_first_row = row;
      } else {
        ++run_count;
        run_first_row = std::min(run_first_row, row);
      }
    }
    finish_run();

    if (best_row >= 0) {
      modes[static_cast<std::size_t>(group)] = best_value;
    }
  }

  auto out = Series::empty({group_count}, ScalarType::Float64, Platform::CPU);
  write_float64_output_(out, modes);
  return out;
}

template <class Ret, template <class> class Kernel>
Ret dispatch_grouped_numeric_(
    const Series& values,
    const Series& group_ids,
    std::int64_t group_count,
    const char* where) {
  switch (values.scalar_type()) {
    case ScalarType::Bool:
      return Kernel<bool>{}(values, group_ids, group_count);
    case ScalarType::UInt8:
      return Kernel<std::uint8_t>{}(values, group_ids, group_count);
    case ScalarType::UInt16:
      return Kernel<std::uint16_t>{}(values, group_ids, group_count);
    case ScalarType::UInt32:
      return Kernel<std::uint32_t>{}(values, group_ids, group_count);
    case ScalarType::UInt64:
      return Kernel<std::uint64_t>{}(values, group_ids, group_count);
    case ScalarType::Int8:
      return Kernel<std::int8_t>{}(values, group_ids, group_count);
    case ScalarType::Int16:
      return Kernel<std::int16_t>{}(values, group_ids, group_count);
    case ScalarType::Int32:
      return Kernel<std::int32_t>{}(values, group_ids, group_count);
    case ScalarType::Int64:
      return Kernel<std::int64_t>{}(values, group_ids, group_count);
    case ScalarType::Float8E4M3FN:
      return Kernel<Float8E4M3FN>{}(values, group_ids, group_count);
    case ScalarType::Float8E5M2:
      return Kernel<Float8E5M2>{}(values, group_ids, group_count);
    case ScalarType::Float16:
      return Kernel<Float16>{}(values, group_ids, group_count);
    case ScalarType::BFloat16:
      return Kernel<BFloat16>{}(values, group_ids, group_count);
    case ScalarType::Float32:
      return Kernel<float>{}(values, group_ids, group_count);
    case ScalarType::Float64:
      return Kernel<double>{}(values, group_ids, group_count);
    default:
      PLUSS_FAIL(std::string(where) + ": dtype not implemented");
  }
}

template <class T>
struct MedianKernel {
  Series operator()(
      const Series& values,
      const Series& group_ids,
      std::int64_t group_count) const {
    return grouped_median_impl_<T>(values, group_ids, group_count);
  }
};

template <class T>
struct MedianBoundsKernel {
  std::pair<Series, Series> operator()(
      const Series& values,
      const Series& group_ids,
      std::int64_t group_count) const {
    std::vector<double> lower, upper;
    grouped_median_bounds_impl_<T>(values, group_ids, group_count, lower, upper);
    auto lower_out = Series::empty({group_count}, ScalarType::Float64, Platform::CPU);
    auto upper_out = Series::empty({group_count}, ScalarType::Float64, Platform::CPU);
    write_float64_output_(lower_out, lower);
    write_float64_output_(upper_out, upper);
    return {lower_out, upper_out};
  }
};

template <class T>
struct ModeKernel {
  Series operator()(
      const Series& values,
      const Series& group_ids,
      std::int64_t group_count) const {
    return grouped_mode_impl_<T>(values, group_ids, group_count);
  }
};

} // namespace

Series grouped_median(
    const Series& values,
    const Series& group_ids,
    std::int64_t group_count) {
  require_grouped_args_(
      values,
      group_ids,
      group_count,
      "native::cpu::grouped_median");
  return dispatch_grouped_numeric_<Series, MedianKernel>(
      values,
      group_ids,
      group_count,
      "native::cpu::grouped_median");
}

std::pair<Series, Series> grouped_median_bounds(
    const Series& values,
    const Series& group_ids,
    std::int64_t group_count) {
  require_grouped_args_(
      values,
      group_ids,
      group_count,
      "native::cpu::grouped_median_bounds");
  return dispatch_grouped_numeric_<std::pair<Series, Series>, MedianBoundsKernel>(
      values,
      group_ids,
      group_count,
      "native::cpu::grouped_median_bounds");
}

Series grouped_mode(
    const Series& values,
    const Series& group_ids,
    std::int64_t group_count) {
  require_grouped_args_(
      values,
      group_ids,
      group_count,
      "native::cpu::grouped_mode");
  return dispatch_grouped_numeric_<Series, ModeKernel>(
      values,
      group_ids,
      group_count,
      "native::cpu::grouped_mode");
}

} // namespace pluss::codem::native::cpu
