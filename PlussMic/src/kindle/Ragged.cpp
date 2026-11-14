#include "kindle/Ragged.h"

#include <algorithm>


#include "CodeM/core/Errors.h"
#include "kindle/catalog.h"
#include <pluss/tils/kit/output.h>

namespace pluss::kindle {

RaggedPolicy RaggedPolicy::defaults(OpClass op) {
  RaggedPolicy p;  // Longest, Head, Skip, nulls Skip, Preserve, Report, min_support 1
  switch (op) {
    case OpClass::Reduce:
      break;
    case OpClass::Elementwise:
      p.absent = RaggedAbsent::Null;
      break;
    case OpClass::Relational:
      p.absent = RaggedAbsent::Null;  // a row with no partner is an explicit null
      break;
    case OpClass::Pairwise:
      p.domain = RaggedDomain::Shortest;  // complete pairs only
      break;
    case OpClass::Plot:
      break;
    case OpClass::Rectangular:
      p.absent = RaggedAbsent::Null;
      p.materialize = RaggedMaterialize::Pad;
      break;
    case OpClass::Persist:
      p.loss = RaggedLoss::Error;  // storage must not lose anything
      break;
  }
  return p;
}

void RaggedPolicy::validate(std::string_view where) const {
  PLUSS_CHECK(min_support >= 1, pluss::tils::OutputEngine::format("{}: min_support must be >= 1", where));
  PLUSS_CHECK((absent == RaggedAbsent::Fill) == fill.has_value(),
              pluss::tils::OutputEngine::format("{}: a fill value is required exactly when absent == Fill", where));
  PLUSS_CHECK((domain == RaggedDomain::Anchor) == anchor_stream.has_value(),
              pluss::tils::OutputEngine::format("{}: an anchor stream is required exactly when domain == Anchor", where));
}

std::vector<std::int64_t> RaggedLayout::presence() const {
  std::vector<std::int64_t> diff(static_cast<std::size_t>(longest) + 1, 0);
  for (std::size_t s = 0; s < extent.size(); ++s) {
    diff[static_cast<std::size_t>(offset[s])] += 1;
    diff[static_cast<std::size_t>(offset[s] + extent[s])] -= 1;
  }
  std::vector<std::int64_t> out(static_cast<std::size_t>(domain()), 0);
  std::int64_t running = 0;
  for (std::int64_t i = 0; i < window_end; ++i) {
    running += diff[static_cast<std::size_t>(i)];
    if (i >= window_begin) out[static_cast<std::size_t>(i - window_begin)] = running;
  }
  return out;
}

std::int64_t RaggedLayout::shift_of(std::string_view key) const {
  const auto it = std::find(keys.begin(), keys.end(), key);
  PLUSS_CHECK(it != keys.end(), pluss::tils::OutputEngine::format("RaggedLayout: no stream '{}'", key));
  return window_begin - offset[static_cast<std::size_t>(it - keys.begin())];
}

RaggedLayout RaggedLayout::of(const Catalog& catalog, const RaggedPolicy& policy, std::string_view where) {
  policy.validate(where);
  PLUSS_CHECK(catalog.defined(), pluss::tils::OutputEngine::format("{}: undefined catalog", where));
  RaggedLayout out;
  const auto& names = catalog.keys();
  out.keys = names;
  out.extent.reserve(names.size());
  for (const auto& name : names) out.extent.push_back(catalog.keys(name).size());
  if (out.extent.empty()) return out;

  out.longest = *std::max_element(out.extent.begin(), out.extent.end());
  out.shortest = *std::min_element(out.extent.begin(), out.extent.end());
  out.offset.assign(out.extent.size(), 0);
  if (policy.alignment == RaggedAlignment::Tail) {
    for (std::size_t s = 0; s < out.extent.size(); ++s) out.offset[s] = out.longest - out.extent[s];
  }

  switch (policy.domain) {
    case RaggedDomain::Longest:
      out.window_begin = 0;
      out.window_end = out.longest;
      break;
    case RaggedDomain::Shortest:
      out.window_begin = policy.alignment == RaggedAlignment::Tail ? out.longest - out.shortest : 0;
      out.window_end = out.window_begin + out.shortest;
      break;
    case RaggedDomain::Exact: {
      if (out.ragged()) {
        const auto lo = std::min_element(out.extent.begin(), out.extent.end()) - out.extent.begin();
        const auto hi = std::max_element(out.extent.begin(), out.extent.end()) - out.extent.begin();
        PLUSS_FAIL(pluss::tils::OutputEngine::format("{}: streams must have equal length but '{}' has {} records and '{}' has {}",
                               where, out.keys[static_cast<std::size_t>(lo)], out.shortest,
                               out.keys[static_cast<std::size_t>(hi)], out.longest));
      }
      out.window_begin = 0;
      out.window_end = out.longest;
      break;
    }
    case RaggedDomain::Anchor: {
      const auto it = std::find(out.keys.begin(), out.keys.end(), *policy.anchor_stream);
      PLUSS_CHECK(it != out.keys.end(),
                  pluss::tils::OutputEngine::format("{}: anchor stream '{}' is not in the catalog", where, *policy.anchor_stream));
      const auto s = static_cast<std::size_t>(it - out.keys.begin());
      out.window_begin = out.offset[s];
      out.window_end = out.offset[s] + out.extent[s];
      break;
    }
  }
  PLUSS_CHECK(policy.loss != RaggedLoss::Error || out.trimmed() == 0,
              pluss::tils::OutputEngine::format("{}: the policy would cut {} of {} index(es) and loss is set to Error", where,
                          out.trimmed(), out.longest));
  return out;
}

// -- conversions -----------------------------------------------------------------

RaggedPolicy to_ragged(const CatalogReduceOptions& options) {
  RaggedPolicy p = RaggedPolicy::defaults(OpClass::Reduce);
  switch (options.align) {
    case Align::Strict:
      p.domain = RaggedDomain::Exact;
      break;
    case Align::Inner:
      p.domain = RaggedDomain::Shortest;
      break;
    case Align::Outer:
      p.domain = RaggedDomain::Longest;
      break;
    case Align::Left:
    case Align::Right:
      PLUSS_FAIL("to_ragged: Align::Left/Right need a named anchor stream; use RaggedDomain::Anchor");
  }
  p.alignment = options.anchor == RaggedAnchor::Tail ? RaggedAlignment::Tail : RaggedAlignment::Head;
  if (options.align_by == AlignBy::Label) p.alignment = RaggedAlignment::Label;
  p.absent = options.ended == RaggedEnd::Null ? RaggedAbsent::Null : RaggedAbsent::Skip;
  p.nulls = options.skipnull ? RaggedNulls::Skip : RaggedNulls::Propagate;
  p.min_support = options.min_streams;
  if (options.fill_scalar.has_value()) {
    p.absent = RaggedAbsent::Fill;
    p.fill = options.fill_scalar;
  }
  return p;
}

RaggedPolicy to_ragged(const TabelizePolicy& policy) {
  RaggedPolicy p = RaggedPolicy::defaults(OpClass::Rectangular);
  switch (policy.align) {
    case Align::Strict:
      p.domain = RaggedDomain::Exact;
      break;
    case Align::Inner:
      p.domain = RaggedDomain::Shortest;
      break;
    case Align::Outer:
      p.domain = RaggedDomain::Longest;
      break;
    case Align::Left:
    case Align::Right:
      PLUSS_CHECK(policy.anchor.has_value(), "to_ragged: Align::Left/Right need TabelizePolicy::anchor");
      p.domain = RaggedDomain::Anchor;
      p.anchor_stream = policy.anchor;
      break;
  }
  p.alignment = policy.align_by == AlignBy::Label ? RaggedAlignment::Label : RaggedAlignment::Head;
  switch (policy.fill) {
    case FillPolicy::Null:
      p.absent = RaggedAbsent::Null;
      break;
    case FillPolicy::Scalar:
      PLUSS_CHECK(policy.fill_scalar.has_value(), "to_ragged: FillPolicy::Scalar needs fill_scalar");
      p.absent = RaggedAbsent::Fill;
      p.fill = policy.fill_scalar;
      break;
    case FillPolicy::ForwardFill:
      p.absent = RaggedAbsent::ForwardFill;
      break;
    case FillPolicy::BackwardFill:
      p.absent = RaggedAbsent::BackwardFill;
      break;
  }
  return p;
}

TabelizePolicy to_tabelize(const RaggedPolicy& policy, std::string_view where) {
  policy.validate(where);
  PLUSS_CHECK(policy.alignment != RaggedAlignment::Tail,
              pluss::tils::OutputEngine::format("{}: tabelize cannot align by Tail; use Head or Label", where));
  TabelizePolicy out;
  switch (policy.domain) {
    case RaggedDomain::Longest:
      out.align = Align::Outer;
      break;
    case RaggedDomain::Shortest:
      out.align = Align::Inner;
      break;
    case RaggedDomain::Exact:
      out.align = Align::Strict;
      break;
    case RaggedDomain::Anchor:
      out.align = Align::Left;
      out.anchor = policy.anchor_stream;
      break;
  }
  if (policy.absent == RaggedAbsent::Error) out.align = Align::Strict;
  out.align_by = policy.alignment == RaggedAlignment::Label ? AlignBy::Label : AlignBy::Pos;
  switch (policy.absent) {
    case RaggedAbsent::Fill:
      out.fill = FillPolicy::Scalar;
      out.fill_scalar = policy.fill;
      break;
    case RaggedAbsent::ForwardFill:
      out.fill = FillPolicy::ForwardFill;
      break;
    case RaggedAbsent::BackwardFill:
      out.fill = FillPolicy::BackwardFill;
      break;
    default:
      out.fill = FillPolicy::Null;
      break;
  }
  return out;
}

} // namespace pluss::kindle
