#pragma once

#include "pluss/ai/lymph/common.h"

// lymph::balance:: factory namespace for NodeJ's existing BalanceTarget
// (nodej/options.h) -- no NodeJ changes, this file only adds constrained
// entry points on top of a struct that already exists.
//
// BalanceTargetKind has four values (Keep/Equalize/Ratio/Counts), and its
// fields are correlated with which kind is active: growth/neighbors only
// mean anything once kind == Equalize (or, for Ratio/Counts, once a target
// ratio/count is also set); a bare struct literal lets a caller write e.g.
// {.kind = Keep, .growth = ReuseRows} -- silently meaningless, since Keep
// never looks at growth -- which is exactly the shape
// LYMPH_REDESIGN_PLAN.md 2.0 rule 4 says earns a factory instead of a bare
// aggregate literal.
//
// Scoped to what this session's own downstream work has actually
// exercised: Keep and Equalize (both growth modes), the same three balance
// strategies stripped_down_lymph.cpp/loan_mod_lymp.cpp already compare
// side by side. BalanceTargetKind::Ratio/Counts have no factory here yet --
// a disclosed scope cut, not silently dropped: construct a raw BalanceTarget
// literal directly (still available, nothing here removes it) if a Ratio-
// or Counts-driven balance target is needed before this namespace grows to
// cover them.

namespace pluss::ai::lymph {
namespace balance {

// No resampling -- the imbalance is left as-is, typically paired with
// TrainOptions.class_weight = ClassWeightMode::Balanced instead.
[[nodiscard]] inline BalanceTarget keep() {
  return BalanceTarget{.kind = BalanceTargetKind::Keep};
}

// Equalize class counts by duplicating existing minority-class rows
// (cheapest growth mode -- no neighbor search).
[[nodiscard]] inline BalanceTarget equalize_reuse_rows(std::int64_t neighbors = 5) {
  return BalanceTarget{
      .kind = BalanceTargetKind::Equalize,
      .growth = BalanceGrowthKind::ReuseRows,
      .neighbors = neighbors,
  };
}

// Equalize class counts by synthesizing new minority-class rows blended
// from each row's nearest same-class neighbors (SpatialIndex-backed KD-tree
// search under the hood -- see NodeJ/balance/balance.cpp).
[[nodiscard]] inline BalanceTarget equalize_blend_neighbors(std::int64_t neighbors = 5) {
  return BalanceTarget{
      .kind = BalanceTargetKind::Equalize,
      .growth = BalanceGrowthKind::BlendNeighbors,
      .neighbors = neighbors,
  };
}

} // namespace balance
} // namespace pluss::ai::lymph
