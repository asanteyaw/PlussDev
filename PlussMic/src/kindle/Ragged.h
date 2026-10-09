#pragma once

// The one place that decides what a missing record means.
//
// A Catalog's streams may have different lengths. Three different things can
// be "missing", and operations treat them differently:
//   absent      the stream has no record at that index (it ended, or skips it)
//   null        the record exists but its value is invalid
//   no partner  a relational output row has no counterpart on one side
//               (it materializes as null, but is reported separately)
//
// A RaggedPolicy answers the questions an operation has about them; every
// operation class has a documented default (RaggedPolicy::defaults) and every
// departure from it is an explicit field. Older option structs
// (TabelizePolicy, CatalogReduceOptions, ...) remain as sugar and convert to
// and from this one. Design notes: docs/RAGGED_POLICY_DESIGN.md.

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::kindle {

class Catalog;
struct TabelizePolicy;
struct CatalogReduceOptions;

// What kind of operation is asking. Each class has its own sensible default.
enum class OpClass : std::uint8_t {
  Reduce = 0,       // sum/mean/min/max/var/std across streams: reduce what is present
  Elementwise = 1,  // a + b between streams: an absent operand makes the value unknown
  Relational = 2,   // join/merge/pull/traverse: keep every record, report what lacks a partner
  Pairwise = 3,     // corr/cov: complete pairs only
  Plot = 4,         // draw what exists
  Rectangular = 5,  // tabelize/DataFrame/SQL/wide export: must be rectangular, so pad, never trim
  Persist = 6,      // storage: lossless
};

// Which indices exist in a result.
enum class RaggedDomain : std::uint8_t {
  Longest = 0,   // every index up to the longest stream
  Shortest = 1,  // only indices every stream has (lossy: the layout counts what is cut)
  Exact = 2,     // streams must have equal length, otherwise an error
  Anchor = 3,    // follow one named stream's extent (RaggedPolicy::anchor_stream)
};

// What "the same index" means across streams.
enum class RaggedAlignment : std::uint8_t {
  Head = 0,   // first records line up
  Tail = 1,   // last records line up
  Label = 2,  // records line up by their stream labels
};

// What an absent record does to the operation.
enum class RaggedAbsent : std::uint8_t {
  Skip = 0,          // contributes nothing (reductions)
  Null = 1,          // the result there is unknown
  Fill = 2,          // use RaggedPolicy::fill
  ForwardFill = 3,   // repeat the previous record
  BackwardFill = 4,  // use the next record
  Error = 5,         // absence is an error
};

// What an in-range null does.
enum class RaggedNulls : std::uint8_t {
  Skip = 0,       // ignored like an absent record
  Propagate = 1,  // the result at that index is null
};

// For results that need not be rectangular.
enum class RaggedMaterialize : std::uint8_t {
  Preserve = 0,  // stay ragged: only trailing absence is trimmed
  Pad = 1,       // one entry per index, nulls (or the fill) where there is no record
};

enum class RaggedLoss : std::uint8_t {
  Report = 0,  // losses are counted and reported, the operation proceeds
  Error = 1,   // any loss (records dropped, trimmed or discarded) is an error
};

struct RaggedPolicy {
  RaggedDomain domain{RaggedDomain::Longest};
  RaggedAlignment alignment{RaggedAlignment::Head};
  RaggedAbsent absent{RaggedAbsent::Skip};
  RaggedNulls nulls{RaggedNulls::Skip};
  RaggedMaterialize materialize{RaggedMaterialize::Preserve};
  RaggedLoss loss{RaggedLoss::Report};
  std::int64_t min_support{1};            // fewest contributing streams for a value to exist
  std::optional<double> fill{};           // value used when absent == Fill
  std::optional<std::string> anchor_stream{};  // stream followed when domain == Anchor

  // The documented default for an operation class.
  static RaggedPolicy defaults(OpClass op);

  // Throws when the combination is contradictory (Fill without a value, a
  // missing anchor stream, min_support < 1, ...).
  void validate(std::string_view where) const;

  bool operator==(const RaggedPolicy& other) const = default;
};

// How a catalog's streams sit in one shared index space under a policy.
// Index space: [0, longest). Head alignment starts every stream at 0; Tail
// alignment ends every stream at `longest`. The policy keeps the window
// [window_begin, window_end) of it.
struct RaggedLayout {
  std::vector<std::string> keys{};
  std::vector<std::int64_t> extent{};  // records per stream
  std::vector<std::int64_t> offset{};  // where each stream starts in the index space
  std::int64_t longest{0};
  std::int64_t shortest{0};
  std::int64_t window_begin{0};
  std::int64_t window_end{0};

  std::int64_t domain() const { return window_end - window_begin; }
  bool ragged() const { return longest != shortest; }
  // Indices of the longest extent the policy cuts away (0 unless Shortest/Anchor).
  std::int64_t trimmed() const { return longest - domain(); }
  std::int64_t streams() const { return static_cast<std::int64_t>(extent.size()); }

  // For each index of [window_begin, window_end): how many streams have a
  // record there (O(streams + longest), no per-stream sweep).
  std::vector<std::int64_t> presence() const;

  // Stream position of window record r is r + shift_of(stream key).
  std::int64_t shift_of(std::string_view key) const;

  // Resolve the layout. Throws for RaggedDomain::Exact on a ragged catalog,
  // for a missing anchor stream, and (RaggedLoss::Error) when the policy would
  // cut records. A Label alignment is resolved by the operation itself, since
  // it needs the labels; the layout then describes positions.
  static RaggedLayout of(const Catalog& catalog, const RaggedPolicy& policy, std::string_view where);
};

// -- conversions between the policy and the older option structs --------------

// Reductions: align -> domain, ended -> absent, anchor, min_streams -> min_support,
// skipnull -> nulls. fill_scalar / label alignment have no jagged equivalent and
// stay on the padding path (see CatalogReduceOptions).
RaggedPolicy to_ragged(const CatalogReduceOptions& options);

// Tabelize: Strict/Inner/Outer (+ Left with an anchor stream) and the fill modes.
RaggedPolicy to_ragged(const TabelizePolicy& policy);

// The tabelize executor's vocabulary for a policy. Tabelize always returns a
// rectangle, so materialize is not consulted; Tail alignment is not supported
// by it and is rejected rather than approximated.
TabelizePolicy to_tabelize(const RaggedPolicy& policy, std::string_view where);

} // namespace pluss::kindle
