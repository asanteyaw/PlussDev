#include "pluss/subsp/eng/lti/sim.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/TypePromotion.h"
#include "detail/shape.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <string>
#include <vector>

namespace pluss::subsp::eng::lti {
namespace {

std::vector<std::int64_t> with_tail(std::vector<std::int64_t> batch,
                                    std::initializer_list<std::int64_t> tail) {
  batch.insert(batch.end(), tail.begin(), tail.end());
  return batch;
}

Series zeros(const std::vector<std::int64_t> &shape, ScalarType dtype,
             Platform platform) {
  return series::zeros(shape,
                       Config{}.with_dtype(dtype).with_platform(platform));
}

void validate_time(const StateSpace &system, const Series &time) {
  PLUSS_CHECK(time.defined(), "eng::lti::sim: time must be defined");
  PLUSS_CHECK(time.dim() == 1 && time.numel() > 0,
              "eng::lti::sim: time must be a non-empty vector");
  PLUSS_CHECK(time.dtype().is_floating() && !time.dtype().is_complex(),
              "eng::lti::sim: time must have a real floating dtype");
  PLUSS_CHECK(time.platform() == system.a().platform(),
              "eng::lti::sim: time and system must share one platform");
  const auto host =
      time.to(Platform::CPU, ScalarType::Float64).items<double>();
  for (std::size_t lane = 0; lane < host.size(); ++lane) {
    PLUSS_CHECK(std::isfinite(host[lane]),
                "eng::lti::sim: time values must be finite");
    if (lane == 0)
      continue;
    PLUSS_CHECK(host[lane] > host[lane - 1],
                "eng::lti::sim: time values must be strictly increasing");
    if (system.is_disc()) {
      const double step = host[lane] - host[lane - 1];
      const double scale =
          std::max({1.0, std::abs(step), system.sample_time()});
      PLUSS_CHECK(std::abs(step - system.sample_time()) <= 1e-10 * scale,
                  "eng::lti::sim: discrete time increments must equal "
                  "sample_time");
    }
  }
}

Series propagators(const Series &a, const Series &b, const Series &time,
                   const std::vector<std::int64_t> &batch, Hold hold) {
  const std::int64_t steps = time.numel() - 1;
  const std::int64_t states = a.size(-1);
  const std::int64_t inputs = b.size(-1);
  const std::size_t time_axis = batch.size();
  const Series dt = time.slice(0, 1, time.numel())
                        .sub(time.slice(0, 0, time.numel() - 1))
                        .to(a.scalar_type());
  std::vector<std::int64_t> dt_shape(batch.size(), 1);
  dt_shape.insert(dt_shape.end(), {steps, 1, 1});
  const Series scale = dt.reshape(dt_shape);
  const Series at =
      a.unsqueeze(static_cast<std::int64_t>(time_axis)).mul(scale);
  const Series bt =
      b.unsqueeze(static_cast<std::int64_t>(time_axis)).mul(scale);
  std::vector<std::int64_t> interval_batch = batch;
  interval_batch.push_back(steps);

  if (hold == Hold::Zero) {
    const Series top = at.unite({bt}, -1, true);
    const Series bottom =
        zeros(with_tail(interval_batch, {inputs, states + inputs}),
              a.scalar_type(), a.platform());
    return top.unite({bottom}, -2, true).linearalg.matrix_exp();
  }

  const Series right = zeros(with_tail(interval_batch, {states, inputs}),
                             a.scalar_type(), a.platform());
  const Series top = at.unite({bt, right}, -1, true);
  const Series left = zeros(with_tail(interval_batch, {inputs, states}),
                            a.scalar_type(), a.platform());
  const Series middle = zeros(with_tail(interval_batch, {inputs, inputs}),
                              a.scalar_type(), a.platform());
  const ScalarType eye_dtype = a.scalar_type() == ScalarType::Float32 ||
                                       a.scalar_type() == ScalarType::Complex64
                                   ? ScalarType::Float32
                                   : ScalarType::Float64;
  const Series eye = detail::expand_batch(
      series::eye(inputs, inputs,
                  Config{}.with_dtype(eye_dtype).with_platform(a.platform()))
          .to(a.scalar_type()),
      interval_batch, 2);
  const Series mid = left.unite({middle, eye}, -1, true);
  const Series bottom =
      zeros(with_tail(interval_batch, {inputs, states + 2 * inputs}),
            a.scalar_type(), a.platform());
  return top.unite({mid, bottom}, -2, true).linearalg.matrix_exp();
}

Series stack_time(const std::vector<Series> &values, std::int64_t time_axis) {
  const Series first = values.front().unsqueeze(time_axis);
  if (values.size() == 1)
    return first;
  std::vector<Series> tail;
  tail.reserve(values.size() - 1);
  for (std::size_t lane = 1; lane < values.size(); ++lane)
    tail.push_back(values[lane].unsqueeze(time_axis));
  return first.unite(tail, time_axis, true);
}


template <class T>
std::vector<T> host_values(const Series &value, ScalarType dtype) {
  const Series flat = value.to(Platform::CPU, dtype).contiguous();
  return flat.reshape({flat.numel()}).template items<T>();
}

// The step recursion is sequential in time but independent across lanes, so it runs as one tight loop per lane on
// the host. `ad`, `bd0`, `bd1` are the per-interval propagators (continuous time) or A and B (discrete time).
template <class T>
SimRun host_recursion(const StateSpace &system, const Series &a, const Series &b, const Series &c, const Series &d,
                      const Series &u, const Series &state0, const Series &transitions, bool uniform, Hold hold,
                      const std::vector<std::int64_t> &batch, ScalarType dtype) {
  const std::int64_t n = system.states();
  const std::int64_t m = system.inputs();
  const std::int64_t p = system.outputs();
  const std::int64_t steps = u.size(static_cast<std::int64_t>(batch.size()));
  std::int64_t lanes = 1;
  for (const std::int64_t extent : batch) lanes *= extent;
  const bool discrete = system.is_disc();
  const bool linear = !discrete && hold == Hold::Linear;
  const std::vector<T> cv = host_values<T>(c, dtype);
  const std::vector<T> dv = host_values<T>(d, dtype);
  const std::vector<T> uv = host_values<T>(u, dtype);
  std::vector<T> x = host_values<T>(state0, dtype);
  std::vector<T> av, bv, tv;
  std::int64_t columns = 0;
  if (discrete) {
    av = host_values<T>(a, dtype);
    bv = host_values<T>(b, dtype);
  } else if (steps > 1) {
    columns = transitions.size(-1);
    tv = host_values<T>(transitions.slice(-2, 0, n).contiguous(), dtype);
  }
  std::vector<T> y(static_cast<std::size_t>(lanes * steps * p), T{});
  std::vector<T> xs(static_cast<std::size_t>(lanes * steps * n), T{});
  std::vector<T> next(static_cast<std::size_t>(n));
  for (std::int64_t lane = 0; lane < lanes; ++lane) {
    T *state = x.data() + lane * n;
    for (std::int64_t t = 0; t < steps; ++t) {
      const T *input = uv.data() + (lane * steps + t) * m;
      T *out_state = xs.data() + (lane * steps + t) * n;
      T *out_value = y.data() + (lane * steps + t) * p;
      for (std::int64_t i = 0; i < n; ++i) out_state[i] = state[i];
      for (std::int64_t o = 0; o < p; ++o) {
        T acc{};
        for (std::int64_t i = 0; i < n; ++i) acc += cv[static_cast<std::size_t>((lane * p + o) * n + i)] * state[i];
        for (std::int64_t k = 0; k < m; ++k) acc += dv[static_cast<std::size_t>((lane * p + o) * m + k)] * input[k];
        out_value[o] = acc;
      }
      if (t + 1 == steps) continue;
      if (discrete) {
        for (std::int64_t i = 0; i < n; ++i) {
          T acc{};
          for (std::int64_t j = 0; j < n; ++j) acc += av[static_cast<std::size_t>((lane * n + i) * n + j)] * state[j];
          for (std::int64_t k = 0; k < m; ++k) acc += bv[static_cast<std::size_t>((lane * n + i) * m + k)] * input[k];
          next[static_cast<std::size_t>(i)] = acc;
        }
      } else {
        const T *rows = uniform ? tv.data() + lane * n * columns : tv.data() + (lane * (steps - 1) + t) * n * columns;
        const T *following = uv.data() + (lane * steps + t + 1) * m;
        for (std::int64_t i = 0; i < n; ++i) {
          const T *row = rows + i * columns;
          T acc{};
          for (std::int64_t j = 0; j < n; ++j) acc += row[j] * state[j];
          for (std::int64_t k = 0; k < m; ++k) {
            const T held = row[n + k];
            if (linear) {
              const T slope = row[n + m + k];
              acc += (held - slope) * input[k] + slope * following[k];
            } else {
              acc += held * input[k];
            }
          }
          next[static_cast<std::size_t>(i)] = acc;
        }
      }
      for (std::int64_t i = 0; i < n; ++i) state[i] = next[static_cast<std::size_t>(i)];
    }
  }
  std::vector<std::int64_t> y_shape = batch;
  y_shape.insert(y_shape.end(), {steps, p});
  std::vector<std::int64_t> x_shape = batch;
  x_shape.insert(x_shape.end(), {steps, n});
  const Config cfg = Config{}.with_dtype(dtype).with_platform(system.a().platform());
  return {pluss::make_series(y, y_shape, cfg), pluss::make_series(xs, x_shape, cfg)};
}

SimRun dispatch_host(const StateSpace &system, const Series &a, const Series &b, const Series &c, const Series &d,
                     const Series &u, const Series &state0, const Series &transitions, bool uniform, Hold hold,
                     const std::vector<std::int64_t> &batch, ScalarType dtype) {
  if (dtype == ScalarType::Float64) return host_recursion<double>(system, a, b, c, d, u, state0, transitions, uniform, hold, batch, dtype);
  if (dtype == ScalarType::Float32) return host_recursion<float>(system, a, b, c, d, u, state0, transitions, uniform, hold, batch, dtype);
  if (dtype == ScalarType::Complex128) return host_recursion<std::complex<double>>(system, a, b, c, d, u, state0, transitions, uniform, hold, batch, dtype);
  return host_recursion<std::complex<float>>(system, a, b, c, d, u, state0, transitions, uniform, hold, batch, dtype);
}

SimRun simulate(const StateSpace &system, const Series &input,
                const Series &time, const Series *initial,
                const SimOptions &options) {
  validate_time(system, time);
  PLUSS_CHECK(input.defined(), "eng::lti::sim: input must be defined");
  PLUSS_CHECK(input.dim() >= 2,
              "eng::lti::sim: input must have shape [..., time, inputs]");
  PLUSS_CHECK(input.size(-2) == time.numel(),
              "eng::lti::sim: input and time lengths must match");
  PLUSS_CHECK(input.size(-1) == system.inputs(),
              "eng::lti::sim: input width must equal the system input count");
  PLUSS_CHECK(input.dtype().is_floating() || input.dtype().is_complex(),
              "eng::lti::sim: input must have a floating or complex dtype");
  PLUSS_CHECK(input.platform() == system.a().platform(),
              "eng::lti::sim: input and system must share one platform");

  Hold hold = options.hold;
  if (hold == Hold::Auto)
    hold = system.is_cont() ? Hold::Linear : Hold::Zero;
  PLUSS_CHECK(system.is_cont() || hold == Hold::Zero,
              "eng::lti::sim: discrete simulation only supports zero-order "
              "sample holding");

  auto batch =
      detail::broadcast(detail::batch_shape(system.a(), 2),
                        detail::batch_shape(input, 2), "eng::lti::sim");
  ScalarType dtype =
      codem::promote_types(system.a().scalar_type(), input.scalar_type());
  if (initial != nullptr) {
    PLUSS_CHECK(initial->defined(),
                "eng::lti::sim: initial state must be defined");
    PLUSS_CHECK(initial->dim() >= 1 && initial->size(-1) == system.states(),
                "eng::lti::sim: initial state must have shape [..., states]");
    PLUSS_CHECK(
        initial->dtype().is_floating() || initial->dtype().is_complex(),
        "eng::lti::sim: initial state must have a floating or complex dtype");
    PLUSS_CHECK(
        initial->platform() == system.a().platform(),
        "eng::lti::sim: initial state and system must share one platform");
    batch = detail::broadcast(batch, detail::batch_shape(*initial, 1),
                              "eng::lti::sim");
    dtype = codem::promote_types(dtype, initial->scalar_type());
  }

  const Series a = detail::expand_batch(system.a().to(dtype), batch, 2);
  const Series b = detail::expand_batch(system.b().to(dtype), batch, 2);
  const Series c = detail::expand_batch(system.c().to(dtype), batch, 2);
  const Series d = detail::expand_batch(system.d().to(dtype), batch, 2);
  const Series u = detail::expand_batch(input.to(dtype), batch, 2);
  Series state = initial == nullptr
                     ? zeros(with_tail(batch, {system.states()}), dtype,
                             system.a().platform())
                     : detail::expand_batch(initial->to(dtype), batch, 1);
  Series state_col = state.unsqueeze(-1);

  Series transitions = series::undefined();
  // On a uniform time grid every interval has the same propagator, so one matrix exponential serves all of them.
  bool uniform = false;
  if (system.is_cont() && time.numel() > 1) {
    const std::vector<double> grid = time.to(Platform::CPU, ScalarType::Float64).items<double>();
    const double step = grid[1] - grid[0];
    uniform = true;
    for (std::size_t i = 2; i < grid.size(); ++i) {
      if (std::abs((grid[i] - grid[i - 1]) - step) > 1e-12 * std::max(std::abs(step), 1e-300)) {
        uniform = false;
        break;
      }
    }
    if (uniform) {
      const Series first_interval = time.slice(0, 0, 2);
      transitions = propagators(a, b, first_interval, batch, hold);
    } else {
      transitions = propagators(a, b, time, batch, hold);
    }
  }

  // Inputs that record the tape keep the Series recursion below; everything else runs as a host loop per lane.
  const bool tracked = system.a().tracks_slope() || system.b().tracks_slope() || system.c().tracks_slope() ||
                       system.d().tracks_slope() || input.tracks_slope() || (initial != nullptr && initial->tracks_slope());
  if (system.a().platform() == Platform::CPU && !tracked)
    return dispatch_host(system, a, b, c, d, u, state, transitions, uniform, hold, batch, dtype);

  std::vector<Series> outputs;
  std::vector<Series> states;
  outputs.reserve(static_cast<std::size_t>(time.numel()));
  states.reserve(static_cast<std::size_t>(time.numel()));
  const std::int64_t time_axis = static_cast<std::int64_t>(batch.size());
  for (std::int64_t lane = 0; lane < time.numel(); ++lane) {
    const Series sample = u.select(time_axis, lane).unsqueeze(-1);
    states.push_back(state_col.squeeze(-1));
    outputs.push_back(c.matmul(state_col).add(d.matmul(sample)).squeeze(-1));
    if (lane + 1 == time.numel())
      continue;

    if (system.is_disc()) {
      state_col = a.matmul(state_col).add(b.matmul(sample));
      continue;
    }

    const Series transition = transitions.select(time_axis, uniform ? 0 : lane);
    const Series ad =
        transition.slice(-2, 0, system.states()).slice(-1, 0, system.states());
    const Series bd0 =
        transition.slice(-2, 0, system.states())
            .slice(-1, system.states(), system.states() + system.inputs());
    if (hold == Hold::Zero) {
      state_col = ad.matmul(state_col).add(bd0.matmul(sample));
      continue;
    }
    const Series bd1 = transition.slice(-2, 0, system.states())
                           .slice(-1, system.states() + system.inputs(),
                                  system.states() + 2 * system.inputs());
    const Series next = u.select(time_axis, lane + 1).unsqueeze(-1);
    state_col = ad.matmul(state_col)
                    .add(bd0.sub(bd1).matmul(sample))
                    .add(bd1.matmul(next));
  }

  return {stack_time(outputs, time_axis), stack_time(states, time_axis)};
}

} // namespace

SimRun sim(const StateSpace &system, const Series &input, const Series &time,
           const SimOptions &options) {
  return simulate(system, input, time, nullptr, options);
}

SimRun sim(const StateSpace &system, const Series &input, const Series &time,
           const Series &initial, const SimOptions &options) {
  return simulate(system, input, time, &initial, options);
}

} // namespace pluss::subsp::eng::lti
