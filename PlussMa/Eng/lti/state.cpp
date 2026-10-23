#include "pluss/subsp/eng/lti/state.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/TypePromotion.h"
#include "detail/shape.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::subsp::eng::lti {
namespace {

void validate_matrix(const Series &value, std::string_view name) {
  PLUSS_CHECK(value.defined(), std::string("eng::lti::state: ") +
                                   std::string(name) + " must be defined");
  PLUSS_CHECK(value.dim() >= 2, std::string("eng::lti::state: ") +
                                    std::string(name) +
                                    " must have at least two dimensions");
  PLUSS_CHECK(value.size(-2) > 0 && value.size(-1) > 0,
              std::string("eng::lti::state: ") + std::string(name) +
                  " must have non-empty matrix axes");
  PLUSS_CHECK(value.dtype().is_floating() || value.dtype().is_complex(),
              std::string("eng::lti::state: ") + std::string(name) +
                  " must have a floating or complex dtype");
}

} // namespace

StateSpace state(const Series &a, const Series &b, const Series &c,
                 const Series &d, double sample_time) {
  validate_matrix(a, "A");
  validate_matrix(b, "B");
  validate_matrix(c, "C");
  validate_matrix(d, "D");
  PLUSS_CHECK(std::isfinite(sample_time) && sample_time >= 0.0,
              "eng::lti::state: sample_time must be finite and non-negative");
  PLUSS_CHECK(a.size(-2) == a.size(-1), "eng::lti::state: A must be square");
  const std::int64_t states = a.size(-1);
  const std::int64_t inputs = b.size(-1);
  const std::int64_t outputs = c.size(-2);
  PLUSS_CHECK(b.size(-2) == states,
              "eng::lti::state: B rows must equal the state count");
  PLUSS_CHECK(c.size(-1) == states,
              "eng::lti::state: C columns must equal the state count");
  PLUSS_CHECK(d.size(-2) == outputs && d.size(-1) == inputs,
              "eng::lti::state: D must have shape [outputs, inputs]");
  PLUSS_CHECK(a.platform() == b.platform() && a.platform() == c.platform() &&
                  a.platform() == d.platform(),
              "eng::lti::state: A, B, C, and D must share one platform");

  auto batch = detail::broadcast(detail::batch_shape(a, 2),
                                 detail::batch_shape(b, 2), "eng::lti::state");
  batch =
      detail::broadcast(batch, detail::batch_shape(c, 2), "eng::lti::state");
  batch =
      detail::broadcast(batch, detail::batch_shape(d, 2), "eng::lti::state");

  ScalarType dtype = codem::promote_types(a.scalar_type(), b.scalar_type());
  dtype = codem::promote_types(dtype, c.scalar_type());
  dtype = codem::promote_types(dtype, d.scalar_type());
  return StateSpace(detail::expand_batch(a.to(dtype), batch, 2),
                    detail::expand_batch(b.to(dtype), batch, 2),
                    detail::expand_batch(c.to(dtype), batch, 2),
                    detail::expand_batch(d.to(dtype), batch, 2), sample_time);
}

Series ctrb(const StateSpace &system) {
  std::vector<Series> blocks;
  blocks.reserve(static_cast<std::size_t>(system.states()));
  Series term = system.b();
  blocks.push_back(term);
  for (std::int64_t power = 1; power < system.states(); ++power) {
    term = system.a().matmul(term);
    blocks.push_back(term);
  }
  const Series first = blocks.front();
  if (blocks.size() == 1)
    return first;
  const std::vector<Series> tail(blocks.begin() + 1, blocks.end());
  return first.unite(tail, first.dim() - 1, true);
}

Series obsv(const StateSpace &system) {
  std::vector<Series> blocks;
  blocks.reserve(static_cast<std::size_t>(system.states()));
  Series term = system.c();
  blocks.push_back(term);
  for (std::int64_t power = 1; power < system.states(); ++power) {
    term = term.matmul(system.a());
    blocks.push_back(term);
  }
  const Series first = blocks.front();
  if (blocks.size() == 1)
    return first;
  const std::vector<Series> tail(blocks.begin() + 1, blocks.end());
  return first.unite(tail, first.dim() - 2, true);
}

Series gram(const StateSpace &system, Gram side,
            pluss::subsp::linearalg::MatrixEqOptions options) {
  const bool reach = side == Gram::Reach;
  const Series driver =
      reach ? system.b().matmul(system.b().transpose(-2, -1).conj())
            : system.c().transpose(-2, -1).conj().matmul(system.c());
  const Series dynamics =
      reach ? system.a() : system.a().transpose(-2, -1).conj();
  return system.is_cont() ? dynamics.linearalg.lyap(driver, options)
                          : dynamics.linearalg.dlyap(driver, options);
}

} // namespace pluss::subsp::eng::lti
