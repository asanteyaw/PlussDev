#pragma once

#include "pluss/codem/codem.h"
#include "pluss/export.h"

namespace pluss::subsp::actuarial {

struct CommutationTable {
  pluss::codem::Series lx = pluss::codem::Series::undefined();  // n+1
  pluss::codem::Series dx = pluss::codem::Series::undefined();  // n
  pluss::codem::Series D = pluss::codem::Series::undefined();   // n+1
  pluss::codem::Series N = pluss::codem::Series::undefined();   // n+1 (tail zero at n)
  pluss::codem::Series C = pluss::codem::Series::undefined();   // n
  pluss::codem::Series M = pluss::codem::Series::undefined();   // n+1 (tail zero at n)
};

// qx is expected to be 1D with values in [0, 1], where qx[t] = P[T in (t, t+1] | T > t].
// Returned survival curve is [1, 1-q0, (1-q0)(1-q1), ...].
CODEM_API pluss::codem::Series survival_curve(const pluss::codem::Series& qx);

// Temporary annuity-due EPV: sum_{k=0}^{n-1} v^k * kpx, where n = qx.numel(), v = 1/(1+i).
CODEM_API pluss::codem::Series annuity_due(const pluss::codem::Series& qx, double interest_rate);

// Temporary annuity-immediate EPV: sum_{k=1}^{n} v^k * kpx.
CODEM_API pluss::codem::Series annuity_immediate(const pluss::codem::Series& qx, double interest_rate);

// Temporary term-insurance EPV (benefit 1 at end of year of death):
// sum_{k=0}^{n-1} v^{k+1} * kpx * qx[k].
CODEM_API pluss::codem::Series term_insurance(const pluss::codem::Series& qx, double interest_rate);

// Pure endowment EPV (benefit 1 at time n on survival): v^n * npx, n = qx.numel().
CODEM_API pluss::codem::Series pure_endowment(const pluss::codem::Series& qx, double interest_rate);

// Endowment insurance EPV = term insurance + pure endowment.
CODEM_API pluss::codem::Series endowment_insurance(const pluss::codem::Series& qx, double interest_rate);

// Life-table lives l_x..l_{x+n} from qx and radix l_x.
CODEM_API pluss::codem::Series lx_from_qx(const pluss::codem::Series& qx, double radix = 100000.0);

// Classical discrete commutation columns on 0..n horizon induced by qx.
CODEM_API CommutationTable commutation_table(const pluss::codem::Series& qx,
                                             double interest_rate,
                                             double radix = 100000.0);

// Net level annual premium (paid in advance while in-force) for unit endowment insurance.
CODEM_API pluss::codem::Series net_level_premium_endowment(const pluss::codem::Series& qx,
                                                           double interest_rate);

// Prospective reserve V_t at durations t=0..n (start-of-year convention, so V_n=1).
CODEM_API pluss::codem::Series reserve_curve_endowment(const pluss::codem::Series& qx,
                                                       double interest_rate);

} // namespace pluss::subsp::actuarial
