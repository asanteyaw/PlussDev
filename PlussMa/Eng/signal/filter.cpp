#include "pluss/subsp/eng/signal/filter.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/TypePromotion.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <complex>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::subsp::eng::signal {
namespace {

std::int64_t norm_dim(std::int64_t dim, std::int64_t rank,
                      std::string_view operation) {
  if (dim < 0)
    dim += rank;
  PLUSS_CHECK(dim >= 0 && dim < rank, std::string("eng::signal::") +
                                          std::string(operation) +
                                          ": dim out of range");
  return dim;
}

void check_signal(const Series &value, std::string_view argument,
                  std::string_view operation) {
  const std::string prefix =
      std::string("eng::signal::") + std::string(operation) + ": ";
  PLUSS_CHECK(value.defined(),
              prefix + std::string(argument) + " must be defined");
  PLUSS_CHECK(value.dim() > 0, prefix + std::string(argument) +
                                   " must have at least one dimension");
  PLUSS_CHECK(value.dtype().is_floating() || value.dtype().is_complex(),
              prefix + std::string(argument) +
                  " must have a floating or complex dtype");
}

ScalarType complex_type(ScalarType dtype) {
  return dtype == ScalarType::Float64 || dtype == ScalarType::Complex128
             ? ScalarType::Complex128
             : ScalarType::Complex64;
}

ScalarType complex_type(ScalarType left, ScalarType right) {
  return complex_type(codem::promote_types(left, right));
}

Series align_kernel(const Series &kernel, std::int64_t rank, std::int64_t dim,
                    std::string_view operation) {
  if (kernel.dim() == rank)
    return kernel;
  PLUSS_CHECK(kernel.dim() == 1,
              std::string("eng::signal::") + std::string(operation) +
                  ": kernel must be one-dimensional or match input rank");
  std::vector<std::int64_t> shape(static_cast<std::size_t>(rank), 1);
  shape[static_cast<std::size_t>(dim)] = kernel.size(0);
  return kernel.reshape(shape);
}

void check_batch_axes(const Series &left, const Series &right, std::int64_t dim,
                      std::string_view operation) {
  for (std::int64_t axis = 0; axis < left.dim(); ++axis) {
    if (axis == dim)
      continue;
    const auto l = left.size(axis);
    const auto r = right.size(axis);
    PLUSS_CHECK(l == r || l == 1 || r == 1,
                std::string("eng::signal::") + std::string(operation) +
                    ": non-sample axes are not broadcastable");
  }
}

Series crop_conv(Series full, std::int64_t input_size, std::int64_t kernel_size,
                 std::int64_t dim, ConvMode mode) {
  if (mode == ConvMode::Full)
    return full;
  if (mode == ConvMode::Same) {
    const std::int64_t start = (kernel_size - 1) / 2;
    return full.slice(dim, start, start + input_size);
  }
  const std::int64_t start = std::min(input_size, kernel_size) - 1;
  const std::int64_t size = std::max(input_size, kernel_size) - start;
  return full.slice(dim, start, start + size);
}

Series eval_delay_poly(const Series &coefficients, const Series &omega,
                       std::int64_t dim, ScalarType dtype,
                       std::string_view operation) {
  dim = norm_dim(dim, coefficients.dim(), operation);
  const Series last = coefficients.moveaxis(dim, -1).to(dtype);
  const std::int64_t count = last.size(-1);
  PLUSS_CHECK(count > 0, std::string("eng::signal::") + std::string(operation) +
                             ": coefficient axis must be non-empty");
  const ScalarType real_dtype = dtype == ScalarType::Complex128
                                    ? ScalarType::Float64
                                    : ScalarType::Float32;
  const auto cfg =
      Config{}.with_dtype(real_dtype).with_platform(coefficients.platform());
  const Series delays =
      pluss::series::arange(0, count, cfg).reshape({1, count});
  const Series phase = omega.to(real_dtype)
                           .reshape({omega.numel(), 1})
                           .mul(delays)
                           .neg()
                           .to(dtype);
  const Series imaginary =
      pluss::series::make(
          std::vector<std::complex<double>>{{0.0, 1.0}},
          Config{}.with_dtype(dtype).with_platform(Platform::CPU))
          .reshape({})
          .to(coefficients.platform());
  const Series roots = phase.mul(imaginary).exp();
  return last.unsqueeze(-2).mul(roots).sum(-1, false);
}

Config config_for(const Series &like, ScalarType dtype) {
  return Config{}.with_dtype(dtype).with_platform(like.platform());
}

Series join(const std::vector<Series> &values, std::int64_t dim) {
  PLUSS_CHECK(!values.empty(), "eng::signal: cannot join an empty list");
  if (values.size() == 1)
    return values.front().unsqueeze(dim);
  const std::vector<Series> tail(values.begin() + 1, values.end());
  return values.front().pile(tail, dim);
}

std::vector<std::int64_t> state_shape(const Series &input, std::int64_t dim,
                                      std::int64_t order) {
  std::vector<std::int64_t> shape;
  shape.reserve(static_cast<std::size_t>(input.dim()));
  for (std::int64_t axis = 0; axis < input.dim(); ++axis) {
    if (axis != dim)
      shape.push_back(input.size(axis));
  }
  shape.push_back(order);
  return shape;
}

Series fit_state(const Series &state, const std::vector<std::int64_t> &target,
                 const Series &input, ScalarType dtype,
                 std::string_view operation) {
  const std::string prefix =
      std::string("eng::signal::") + std::string(operation) + ": ";
  PLUSS_CHECK(state.defined(), prefix + "state must be defined");
  PLUSS_CHECK(state.platform() == input.platform(),
              prefix + "state and input must share one platform");
  PLUSS_CHECK(state.dim() <= static_cast<std::int64_t>(target.size()),
              prefix + "state rank is too large");

  std::vector<std::int64_t> aligned(
      target.size() - static_cast<std::size_t>(state.dim()), 1);
  aligned.insert(aligned.end(), state.sizes().begin(), state.sizes().end());
  for (std::size_t axis = 0; axis < target.size(); ++axis) {
    PLUSS_CHECK(aligned[axis] == 1 || aligned[axis] == target[axis],
                prefix + "state is not broadcastable to the signal lanes");
  }
  return state.to(dtype).reshape(aligned).expand(target, true);
}

struct Coefficients {
  Series b;
  Series a;
  std::int64_t order;
  ScalarType dtype;
};

Coefficients coefficients(const Series &input, const Series &numerator,
                          const Series &denominator, const Series *state,
                          std::string_view operation) {
  check_signal(input, "input", operation);
  check_signal(numerator, "numerator", operation);
  check_signal(denominator, "denominator", operation);
  const std::string prefix =
      std::string("eng::signal::") + std::string(operation) + ": ";
  PLUSS_CHECK(numerator.dim() == 1 && numerator.numel() > 0,
              prefix + "numerator must be a non-empty vector");
  PLUSS_CHECK(denominator.dim() == 1 && denominator.numel() > 0,
              prefix + "denominator must be a non-empty vector");
  PLUSS_CHECK(input.platform() == numerator.platform() &&
                  input.platform() == denominator.platform(),
              prefix + "input and coefficients must share one platform");
  if (state != nullptr) {
    PLUSS_CHECK(state->defined(), prefix + "state must be defined");
    PLUSS_CHECK(state->platform() == input.platform(),
                prefix + "state and input must share one platform");
  }

  ScalarType dtype =
      codem::promote_types(input.scalar_type(), numerator.scalar_type());
  dtype = codem::promote_types(dtype, denominator.scalar_type());
  if (state != nullptr)
    dtype = codem::promote_types(dtype, state->scalar_type());
  PLUSS_CHECK(dtype_of(dtype).is_floating() || dtype_of(dtype).is_complex(),
              prefix + "promoted dtype must be floating or complex");

  Series b = numerator.to(dtype);
  Series a = denominator.to(dtype);
  PLUSS_CHECK(!a.select(0, 0).eq(0.0).item<bool>(),
              prefix + "denominator[0] must be nonzero");
  const Series a0 = a.select(0, 0);
  b = b.div(a0);
  a = a.div(a0);

  const std::int64_t width = std::max(b.size(0), a.size(0));
  const auto cfg = config_for(input, dtype);
  if (b.size(0) < width)
    b = b.unite({pluss::series::zeros({width - b.size(0)}, cfg)}, 0);
  if (a.size(0) < width)
    a = a.unite({pluss::series::zeros({width - a.size(0)}, cfg)}, 0);
  return {std::move(b), std::move(a), width - 1, dtype};
}

FilterRun filter_impl(const Series &input, const Series &numerator,
                      const Series &denominator, const Series *initial,
                      std::int64_t dim, std::string_view operation) {
  const Coefficients coeff =
      coefficients(input, numerator, denominator, initial, operation);
  dim = norm_dim(dim, input.dim(), operation);
  const auto target = state_shape(input, dim, coeff.order);
  Series state =
      initial == nullptr
          ? pluss::series::zeros(target, config_for(input, coeff.dtype))
          : fit_state(*initial, target, input, coeff.dtype, operation);
  const Series samples = input.to(coeff.dtype).moveaxis(dim, -1);
  const std::int64_t count = samples.size(-1);
  if (count == 0)
    return {samples.moveaxis(-1, dim), std::move(state)};

  std::vector<Series> b_terms;
  std::vector<Series> a_terms;
  b_terms.reserve(static_cast<std::size_t>(coeff.order + 1));
  a_terms.reserve(static_cast<std::size_t>(coeff.order + 1));
  for (std::int64_t lane = 0; lane <= coeff.order; ++lane) {
    b_terms.push_back(coeff.b.select(0, lane));
    a_terms.push_back(coeff.a.select(0, lane));
  }

  std::vector<Series> output;
  output.reserve(static_cast<std::size_t>(count));
  for (std::int64_t sample = 0; sample < count; ++sample) {
    const Series x = samples.select(-1, sample);
    Series y = x.mul(b_terms.front());
    if (coeff.order > 0)
      y = y.add(state.select(-1, 0));
    output.push_back(y);

    if (coeff.order == 0)
      continue;
    std::vector<Series> next;
    next.reserve(static_cast<std::size_t>(coeff.order));
    for (std::int64_t lane = 0; lane < coeff.order; ++lane) {
      Series value =
          x.mul(b_terms[static_cast<std::size_t>(lane + 1)])
              .sub(y.mul(a_terms[static_cast<std::size_t>(lane + 1)]));
      if (lane + 1 < coeff.order)
        value = value.add(state.select(-1, lane + 1));
      next.push_back(std::move(value));
    }
    state = join(next, -1);
  }
  return {join(output, -1).moveaxis(-1, dim), std::move(state)};
}

void check_sections(const Series &input, const Series &sections,
                    std::string_view operation) {
  check_signal(input, "input", operation);
  check_signal(sections, "sections", operation);
  const std::string prefix =
      std::string("eng::signal::") + std::string(operation) + ": ";
  PLUSS_CHECK(sections.dim() == 2 && sections.size(0) > 0 &&
                  sections.size(1) == 6,
              prefix + "sections must have shape [section, 6]");
  PLUSS_CHECK(input.platform() == sections.platform(),
              prefix + "input and sections must share one platform");
  PLUSS_CHECK(!sections.select(1, 3).eq(0.0).any().item<bool>(),
              prefix + "every section denominator[0] must be nonzero");
}

FilterRun sos_impl(const Series &input, const Series &sections,
                   const Series *initial, std::int64_t dim,
                   std::string_view operation) {
  check_sections(input, sections, operation);
  dim = norm_dim(dim, input.dim(), operation);
  ScalarType dtype =
      codem::promote_types(input.scalar_type(), sections.scalar_type());
  if (initial != nullptr)
    dtype = codem::promote_types(dtype, initial->scalar_type());
  const std::int64_t count = sections.size(0);
  auto target = state_shape(input, dim, 2);
  target.insert(target.end() - 1, count);
  Series state = initial == nullptr
                     ? pluss::series::zeros(target, config_for(input, dtype))
                     : fit_state(*initial, target, input, dtype, operation);

  Series values = input.to(dtype);
  std::vector<Series> final_state;
  final_state.reserve(static_cast<std::size_t>(count));
  const Series bank = sections.to(dtype);
  for (std::int64_t section = 0; section < count; ++section) {
    const Series row = bank.select(0, section);
    const Series b = row.slice(0, 0, 3);
    const Series a = row.slice(0, 3, 6);
    const Series section_state = state.select(-2, section);
    FilterRun run = filter_impl(values, b, a, &section_state, dim, operation);
    values = std::move(run.values);
    final_state.push_back(std::move(run.state));
  }
  return {std::move(values), join(final_state, -2)};
}

Series steady_state(const Series &input, const Series &numerator,
                    const Series &denominator, std::string_view operation) {
  const Coefficients coeff =
      coefficients(input, numerator, denominator, nullptr, operation);
  if (coeff.order == 0)
    return pluss::series::zeros({0}, config_for(input, coeff.dtype));
  PLUSS_CHECK(!coeff.a.sum().eq(0.0).item<bool>(),
              std::string("eng::signal::") + std::string(operation) +
                  ": denominator sum must be nonzero for steady state");

  const Series tail_a = coeff.a.slice(0, 1, coeff.order + 1);
  const Series rhs = coeff.b.slice(0, 1, coeff.order + 1)
                         .sub(tail_a.mul(coeff.b.select(0, 0)))
                         .unsqueeze(-1);
  const auto cfg = config_for(input, coeff.dtype);
  const Series first = tail_a.neg().unsqueeze(-1);
  const Series shift = pluss::series::eye(coeff.order, coeff.order - 1, cfg);
  const Series transition = first.unite({shift}, 1);
  const Series system = pluss::series::eye(coeff.order, cfg).sub(transition);
  return system.linearalg.inverse().matmul(rhs).squeeze(-1);
}

std::int64_t filter_order(const Series &numerator, const Series &denominator) {
  const auto effective_width = [](const Series &value) {
    const Series cpu = value.to(Platform::CPU);
    const Series positions =
        pluss::series::arange(1, cpu.size(0) + 1, ScalarType::Int64);
    return cpu.ne(0.0)
        .to(ScalarType::Int64)
        .mul(positions)
        .max()
        .item<std::int64_t>();
  };
  return std::max(effective_width(numerator), effective_width(denominator)) - 1;
}

std::int64_t sos_order(const Series &sections) {
  const Series cpu = sections.to(Platform::CPU);
  const Series positions =
      pluss::series::arange(1, 4, ScalarType::Int64).reshape({1, 3});
  const Series b_width = cpu.slice(1, 0, 3)
                             .ne(0.0)
                             .to(ScalarType::Int64)
                             .mul(positions)
                             .max(1, false);
  const Series a_width = cpu.slice(1, 3, 6)
                             .ne(0.0)
                             .to(ScalarType::Int64)
                             .mul(positions)
                             .max(1, false);
  return b_width.maximum(a_width).sub(1).sum().item<std::int64_t>();
}

std::int64_t edge_size(std::int64_t order, std::int64_t count,
                       const PhaseOpts &options, std::string_view operation) {
  const std::string prefix =
      std::string("eng::signal::") + std::string(operation) + ": ";
  PLUSS_CHECK(options.edge >= -1, prefix + "edge must be -1 or nonnegative");
  const std::int64_t edge = options.pad == PadMode::None ? 0
                            : options.edge >= 0
                                ? options.edge
                                : std::max<std::int64_t>(1, 3 * order);
  PLUSS_CHECK(edge < count,
              prefix + "sample axis must be longer than the edge extension");
  return edge;
}

Series extend(const Series &input, std::int64_t edge, PadMode mode) {
  if (edge == 0 || mode == PadMode::None)
    return input;
  const std::int64_t count = input.size(-1);
  const Series left_core = input.slice(-1, 1, edge + 1).flip(-1);
  const Series right_core =
      input.slice(-1, count - edge - 1, count - 1).flip(-1);
  Series left;
  Series right;
  if (mode == PadMode::Odd) {
    left = input.select(-1, 0).unsqueeze(-1).mul(2.0).sub(left_core);
    right = input.select(-1, count - 1).unsqueeze(-1).mul(2.0).sub(right_core);
  } else if (mode == PadMode::Even) {
    left = left_core;
    right = right_core;
  } else {
    std::vector<std::int64_t> shape(input.sizes().begin(), input.sizes().end());
    shape.back() = edge;
    left = input.select(-1, 0).unsqueeze(-1).expand(shape, true);
    right = input.select(-1, count - 1).unsqueeze(-1).expand(shape, true);
  }
  return left.unite({input, right}, -1);
}

Series trim_phase(Series values, std::int64_t edge, std::int64_t dim) {
  values = values.flip(dim);
  if (edge == 0)
    return values;
  return values.slice(dim, edge, values.size(dim) - edge);
}

} // namespace

Series conv(const Series &input, const Series &kernel, std::int64_t dim,
            ConvMode mode) {
  check_signal(input, "input", "conv");
  check_signal(kernel, "kernel", "conv");
  PLUSS_CHECK(input.platform() == kernel.platform(),
              "eng::signal::conv: input and kernel must share one platform");
  dim = norm_dim(dim, input.dim(), "conv");
  const Series aligned = align_kernel(kernel, input.dim(), dim, "conv");
  check_batch_axes(input, aligned, dim, "conv");
  const std::int64_t input_size = input.size(dim);
  const std::int64_t kernel_size = aligned.size(dim);
  PLUSS_CHECK(input_size > 0 && kernel_size > 0,
              "eng::signal::conv: sample axes must be non-empty");
  PLUSS_CHECK(input_size <=
                  std::numeric_limits<std::int64_t>::max() - kernel_size + 1,
              "eng::signal::conv: output length overflows int64");

  const ScalarType dtype =
      codem::promote_types(input.scalar_type(), aligned.scalar_type());
  const bool real = !dtype_of(dtype).is_complex();
  const std::int64_t full_size = input_size + kernel_size - 1;
  Series full;
  if (real) {
    full = input.to(dtype)
               .rfft(full_size, dim, "backward")
               .mul(aligned.to(dtype).rfft(full_size, dim, "backward"))
               .irfft(full_size, dim, "backward")
               .to(dtype);
  } else {
    full = input.to(dtype)
               .fft(full_size, dim, "backward")
               .mul(aligned.to(dtype).fft(full_size, dim, "backward"))
               .ifft(full_size, dim, "backward");
  }
  return crop_conv(std::move(full), input_size, kernel_size, dim, mode);
}

Series corr(const Series &input, const Series &kernel, std::int64_t dim,
            ConvMode mode) {
  check_signal(input, "input", "corr");
  check_signal(kernel, "kernel", "corr");
  PLUSS_CHECK(input.platform() == kernel.platform(),
              "eng::signal::corr: input and kernel must share one platform");
  dim = norm_dim(dim, input.dim(), "corr");
  const Series aligned = align_kernel(kernel, input.dim(), dim, "corr");
  return conv(input, aligned.conj().flip(dim), dim, mode);
}

Series fir(const Series &input, const Series &taps, std::int64_t dim) {
  check_signal(input, "input", "fir");
  dim = norm_dim(dim, input.dim(), "fir");
  return conv(input, taps, dim, ConvMode::Full).slice(dim, 0, input.size(dim));
}

FilterRun filter(const Series &input, const Series &numerator,
                 const Series &denominator, std::int64_t dim) {
  return filter_impl(input, numerator, denominator, nullptr, dim, "filter");
}

FilterRun filter(const Series &input, const Series &numerator,
                 const Series &denominator, const Series &state,
                 std::int64_t dim) {
  return filter_impl(input, numerator, denominator, &state, dim, "filter");
}

FilterRun sos(const Series &input, const Series &sections, std::int64_t dim) {
  return sos_impl(input, sections, nullptr, dim, "sos");
}

FilterRun sos(const Series &input, const Series &sections, const Series &state,
              std::int64_t dim) {
  return sos_impl(input, sections, &state, dim, "sos");
}

Series zero_phase(const Series &input, const Series &numerator,
                  const Series &denominator, const PhaseOpts &options) {
  check_signal(input, "input", "zero_phase");
  const std::int64_t dim = norm_dim(options.dim, input.dim(), "zero_phase");
  const auto coeff =
      coefficients(input, numerator, denominator, nullptr, "zero_phase");
  const std::int64_t edge = edge_size(filter_order(coeff.b, coeff.a),
                                      input.size(dim), options, "zero_phase");
  const Series padded =
      extend(input.to(coeff.dtype).moveaxis(dim, -1), edge, options.pad);
  const Series zi = steady_state(padded, coeff.b, coeff.a, "zero_phase");
  const Series start = padded.select(-1, 0).unsqueeze(-1).mul(zi);
  FilterRun forward =
      filter_impl(padded, coeff.b, coeff.a, &start, -1, "zero_phase");
  Series reversed = forward.values.flip(-1);
  const Series back_start = reversed.select(-1, 0).unsqueeze(-1).mul(zi);
  FilterRun backward =
      filter_impl(reversed, coeff.b, coeff.a, &back_start, -1, "zero_phase");
  return trim_phase(std::move(backward.values), edge, -1).moveaxis(-1, dim);
}

Series zero_phase(const Series &input, const Series &sections,
                  const PhaseOpts &options) {
  check_sections(input, sections, "zero_phase");
  const std::int64_t dim = norm_dim(options.dim, input.dim(), "zero_phase");
  const std::int64_t edge =
      edge_size(sos_order(sections), input.size(dim), options, "zero_phase");
  const ScalarType dtype =
      codem::promote_types(input.scalar_type(), sections.scalar_type());
  const Series bank = sections.to(dtype);
  Series values = input.to(dtype);
  const PhaseOpts stage_options{.dim = dim, .edge = edge, .pad = options.pad};
  for (std::int64_t section = 0; section < bank.size(0); ++section) {
    const Series row = bank.select(0, section);
    values = zero_phase(values, row.slice(0, 0, 3), row.slice(0, 3, 6),
                        stage_options);
  }
  return values;
}

Series freqz(const Series &numerator, const Series &omega, std::int64_t dim) {
  check_signal(numerator, "numerator", "freqz");
  check_signal(omega, "omega", "freqz");
  PLUSS_CHECK(omega.dim() == 1 && omega.numel() > 0,
              "eng::signal::freqz: omega must be a non-empty vector");
  PLUSS_CHECK(!omega.dtype().is_complex(),
              "eng::signal::freqz: omega must be real-valued");
  PLUSS_CHECK(
      numerator.platform() == omega.platform(),
      "eng::signal::freqz: coefficients and omega must share one platform");
  const ScalarType dtype =
      complex_type(numerator.scalar_type(), omega.scalar_type());
  return eval_delay_poly(numerator, omega, dim, dtype, "freqz");
}

Series freqz(const Series &numerator, const Series &denominator,
             const Series &omega, std::int64_t dim) {
  check_signal(numerator, "numerator", "freqz");
  check_signal(denominator, "denominator", "freqz");
  check_signal(omega, "omega", "freqz");
  PLUSS_CHECK(omega.dim() == 1 && omega.numel() > 0,
              "eng::signal::freqz: omega must be a non-empty vector");
  PLUSS_CHECK(!omega.dtype().is_complex(),
              "eng::signal::freqz: omega must be real-valued");
  PLUSS_CHECK(numerator.platform() == denominator.platform() &&
                  numerator.platform() == omega.platform(),
              "eng::signal::freqz: all arguments must share one platform");
  ScalarType dtype =
      codem::promote_types(numerator.scalar_type(), denominator.scalar_type());
  dtype = complex_type(dtype, omega.scalar_type());
  const Series top = eval_delay_poly(numerator, omega, dim, dtype, "freqz");
  const Series bottom =
      eval_delay_poly(denominator, omega, dim, dtype, "freqz");
  return top.div(bottom);
}

} // namespace pluss::subsp::eng::signal
