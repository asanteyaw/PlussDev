#pragma once

#include "pluss/codem/codem.h"
#include "pluss/export.h"

namespace pluss::subsp::actuarial::life_contingencies {

// One-year survival probabilities p_x = 1 - q_x.
CODEM_API pluss::codem::Series px_from_qx(const pluss::codem::Series& qx);

// One-year mortality probabilities q_x = 1 - p_x.
CODEM_API pluss::codem::Series qx_from_px(const pluss::codem::Series& px);

// Discrete one-year force approximation mu_x = -log(1 - q_x).
CODEM_API pluss::codem::Series force_of_mortality_from_qx(const pluss::codem::Series& qx);

// Cumulative hazard H_k = sum_{j=0}^{k} mu_j.
CODEM_API pluss::codem::Series cumulative_hazard_from_qx(const pluss::codem::Series& qx);

// Survival path from force vector: [1, exp(-H_0), exp(-H_1), ...].
CODEM_API pluss::codem::Series survival_from_force(const pluss::codem::Series& force);

// PMF of curtate future lifetime K over finite horizon:
// [q0, p0*q1, p0*p1*q2, ..., npx].
CODEM_API pluss::codem::Series future_lifetime_pmf(const pluss::codem::Series& qx);

// Temporary curtate expectation over provided horizon: e = sum_{k=1}^{n} kpx.
CODEM_API pluss::codem::Series curtate_expectation_of_life(const pluss::codem::Series& qx);

// Curtate expectation by attained duration t=0..n, with terminal 0 at t=n.
CODEM_API pluss::codem::Series curtate_expectation_curve(const pluss::codem::Series& qx);

} // namespace pluss::subsp::actuarial::life_contingencies
