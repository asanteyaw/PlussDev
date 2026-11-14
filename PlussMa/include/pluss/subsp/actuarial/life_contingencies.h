#pragma once

#include "pluss/codem/codem.h"
#include "pluss/export.h"

#include <cstdint>

namespace pluss::subsp::actuarial::life_contingencies {

// Gompertz force surface mu_x = b * c^x over attained ages.
CODEM_API pluss::codem::Series
gompertz_force(const pluss::codem::Series &attained_ages, double b, double c);

// Gompertz one-year mortality surface q_x = 1 - exp(-mu_x).
CODEM_API pluss::codem::Series
gompertz_qx(const pluss::codem::Series &attained_ages, double b, double c);

// Makeham force surface mu_x = a + b * c^x over attained ages.
CODEM_API pluss::codem::Series
makeham_force(const pluss::codem::Series &attained_ages, double a, double b,
              double c);

// Makeham one-year mortality surface q_x = 1 - exp(-mu_x).
CODEM_API pluss::codem::Series
makeham_qx(const pluss::codem::Series &attained_ages, double a, double b,
           double c);

// One-year survival probabilities p_x = 1 - q_x.
CODEM_API pluss::codem::Series px_qx(const pluss::codem::Series &qx);

// One-year mortality probabilities q_x = 1 - p_x.
CODEM_API pluss::codem::Series qx_px(const pluss::codem::Series &px);

// One-year mortality probabilities from force mu_x: q_x = 1 - exp(-mu_x).
CODEM_API pluss::codem::Series qx_force(const pluss::codem::Series &force);

// Discrete one-year force approximation mu_x = -log(1 - q_x).
CODEM_API pluss::codem::Series force_qx(const pluss::codem::Series &qx);

// Cumulative hazard H_k = sum_{j=0}^{k} mu_j.
CODEM_API pluss::codem::Series hazard_qx(const pluss::codem::Series &qx);

// Survival path from force vector: [1, exp(-H_0), exp(-H_1), ...].
CODEM_API pluss::codem::Series
survival_force(const pluss::codem::Series &force);

// PMF of curtate future lifetime K over finite horizon:
// [q0, p0*q1, p0*p1*q2, ..., npx].
CODEM_API pluss::codem::Series lifetime_pmf(const pluss::codem::Series &qx);

// Temporary curtate expectation over provided horizon: e = sum_{k=1}^{n} kpx.
CODEM_API pluss::codem::Series curtate_mean(const pluss::codem::Series &qx);

// Curtate expectation by attained duration t=0..n, with terminal 0 at t=n.
CODEM_API pluss::codem::Series curtate_curve(const pluss::codem::Series &qx);

// Life-table lives l_x..l_{x+n} from q_x and radix l_x.
CODEM_API pluss::codem::Series lx_qx(const pluss::codem::Series &qx,
                                     double radix = 100000.0);

// Life-table lives l_x..l_{x+n} from p_x and radix l_x.
CODEM_API pluss::codem::Series lx_px(const pluss::codem::Series &px,
                                     double radix = 100000.0);

// Life-table decrements d_x = l_x - l_{x+1}.
CODEM_API pluss::codem::Series dx_lx(const pluss::codem::Series &lx);

// Deterministic transform p_x = l_{x+1} / l_x with strict table contracts.
CODEM_API pluss::codem::Series px_lx(const pluss::codem::Series &lx);

// Deterministic transform q_x = 1 - l_{x+1} / l_x with strict table contracts.
CODEM_API pluss::codem::Series qx_lx(const pluss::codem::Series &lx);

// Compose a finite-horizon q_x path from select and ultimate tables.
// First min(select_qx.numel(), projection_years) values come from select_qx.
// Tail values come from ultimate_qx (broadcast if ultimate_qx has length 1).
CODEM_API pluss::codem::Series
select_qx(const pluss::codem::Series &select_rates,
          const pluss::codem::Series &ultimate_qx,
          std::int64_t projection_years);

} // namespace pluss::subsp::actuarial::life_contingencies
