#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::lymph;

bool run_balance_factory_checks(std::ostream* err) {
  // keep(): no resampling, no growth mode set.
  const BalanceTarget kept = balance::keep();
  if (kept.kind != BalanceTargetKind::Keep) {
    if (err) *err << "balance::keep() should set kind = Keep\n";
    return false;
  }

  // equalize_reuse_rows(n): kind/growth/neighbors all set correctly, and
  // the invalid kind=Keep+growth=ReuseRows combination a bare struct
  // literal could accidentally express is simply not reachable through
  // either factory.
  const BalanceTarget reuse = balance::equalize_reuse_rows(7);
  if (reuse.kind != BalanceTargetKind::Equalize || reuse.growth != BalanceGrowthKind::ReuseRows ||
      reuse.neighbors != 7) {
    if (err) *err << "balance::equalize_reuse_rows(7) should set kind=Equalize, growth=ReuseRows, neighbors=7\n";
    return false;
  }

  // equalize_blend_neighbors(n): same shape, different growth mode.
  const BalanceTarget blend = balance::equalize_blend_neighbors(3);
  if (blend.kind != BalanceTargetKind::Equalize || blend.growth != BalanceGrowthKind::BlendNeighbors ||
      blend.neighbors != 3) {
    if (err) *err << "balance::equalize_blend_neighbors(3) should set kind=Equalize, growth=BlendNeighbors, neighbors=3\n";
    return false;
  }

  // Default neighbors argument matches BalanceTarget's own struct default (5).
  const BalanceTarget default_neighbors = balance::equalize_reuse_rows();
  if (default_neighbors.neighbors != 5) {
    if (err) *err << "balance::equalize_reuse_rows() with no argument should default neighbors to 5\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph balance:: factories", "[lymph][balance]") {
  if (!run_balance_factory_checks(&std::cerr)) {
    FAIL("lymph balance factory checks failed");
  }
}
#else
int main() {
  if (!run_balance_factory_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph balance factory checks passed\n";
  return 0;
}
#endif
