#pragma once

#include "pluss/codem/codem.h"
#include "pluss/export.h"
#include "pluss/subsp/actuarial/operator.h"

namespace pluss::subsp::actuarial {

using LossTransform =
    Operator<pluss::codem::Series(const pluss::codem::Series&)>;
using LossMeasure =
    Operator<pluss::codem::Series(const pluss::codem::Series&)>;

namespace operators {

// Retained loss after an ordinary deductible: (X - deductible)+.
[[nodiscard]] CODEM_API LossTransform deductible(double amount);

// Retained loss after an ordinary policy limit: min(X, amount).
[[nodiscard]] CODEM_API LossTransform limit(double amount);

// Proportional participation in a loss or already transformed loss.
[[nodiscard]] CODEM_API LossTransform coinsurance(double share);

// Reinsurance layer: share * min((X - attachment)+, limit).
[[nodiscard]] CODEM_API LossTransform layer(double attachment,
                                           double limit,
                                           double share = 1.0);

// Empirical mean of a loss sample. Empty samples produce a scalar zero.
[[nodiscard]] CODEM_API LossMeasure mean();

} // namespace operators

} // namespace pluss::subsp::actuarial
