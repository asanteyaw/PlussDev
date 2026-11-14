#pragma once

#include "pluss/codem/codem.h"
#include "pluss/export.h"

namespace pluss::subsp::actuarial::actuarial_models {

// Elementwise expected claim cost: exposure * frequency * severity.
CODEM_API pluss::codem::Series
claim_cost(const pluss::codem::Series &exposure,
           const pluss::codem::Series &claim_frequency,
           const pluss::codem::Series &claim_severity);

// Pure premium estimator: sum(losses) / sum(exposure).
CODEM_API pluss::codem::Series
pure_premium(const pluss::codem::Series &incurred_losses,
             const pluss::codem::Series &exposure);

// Loss ratio estimator: sum(losses) / sum(earned_premium).
CODEM_API pluss::codem::Series
loss_ratio(const pluss::codem::Series &incurred_losses,
           const pluss::codem::Series &earned_premium);

// Elementwise pure premium with zero-safe denominator handling:
// losses / exposure, returning 0 where exposure == 0.
CODEM_API pluss::codem::Series
premium_curve(const pluss::codem::Series &incurred_losses,
              const pluss::codem::Series &exposure);

// Elementwise loss ratio with zero-safe denominator handling:
// losses / earned_premium, returning 0 where earned_premium == 0.
CODEM_API pluss::codem::Series
ratio_curve(const pluss::codem::Series &incurred_losses,
            const pluss::codem::Series &earned_premium);

// Bühlmann-style credibility weight z = exposure / (exposure + k), with k > 0.
CODEM_API pluss::codem::Series
credibility_weight(const pluss::codem::Series &exposure, double k);

// Credibility blend: z * experience + (1 - z) * manual.
CODEM_API pluss::codem::Series
credibility_blend(const pluss::codem::Series &experience_pure_premium,
                  const pluss::codem::Series &manual_pure_premium,
                  const pluss::codem::Series &exposure, double credibility_k);

// Indicated premium from target pure premium and load assumptions:
// (pure_premium + fixed_expense) / (1 - variable_expense_ratio - profit_ratio).
CODEM_API pluss::codem::Series
indicated_premium(const pluss::codem::Series &pure_premium,
                  double fixed_expense, double variable_expense_ratio,
                  double profit_ratio);

// Elementwise indicated rate change factor: indicated / current - 1.
CODEM_API pluss::codem::Series
rate_change(const pluss::codem::Series &current_premium,
            const pluss::codem::Series &indicated_premium);

// Ratemaking workflow helper:
// premium_curve(losses/exposure) -> indicated_premium(...) -> rate_change(...).
CODEM_API pluss::codem::Series
experience_change(const pluss::codem::Series &incurred_losses,
                  const pluss::codem::Series &exposure,
                  const pluss::codem::Series &current_premium,
                  double fixed_expense, double variable_expense_ratio,
                  double profit_ratio);

// Bornhuetter-Ferguson ultimate estimate:
// reported + expected_ultimate * (1 - percent_reported).
CODEM_API pluss::codem::Series
bf_ultimate(const pluss::codem::Series &reported_losses,
            const pluss::codem::Series &expected_ultimate_losses,
            const pluss::codem::Series &percent_reported);

// Bornhuetter-Ferguson reserve estimate:
// expected_ultimate * (1 - percent_reported).
CODEM_API pluss::codem::Series
bf_reserve(const pluss::codem::Series &expected_ultimate_losses,
           const pluss::codem::Series &percent_reported);

// Age-to-age (link) factors from a cumulative claims triangle (origin x
// development).
CODEM_API pluss::codem::Series
age_factors(const pluss::codem::Series &cumulative_triangle);

// CDF-to-ultimate factors from age-to-age factors; output length is
// age_to_age.numel() + 1.
CODEM_API pluss::codem::Series
ladder_cdf(const pluss::codem::Series &age_to_age_factors);

// Chain ladder ultimate estimates by origin.
CODEM_API pluss::codem::Series
ladder_ultimate(const pluss::codem::Series &cumulative_triangle,
                const pluss::codem::Series &cdf_to_ultimate);

// Chain ladder reserve estimates by origin = ultimate - latest observed
// cumulative.
CODEM_API pluss::codem::Series
ladder_reserve(const pluss::codem::Series &cumulative_triangle,
               const pluss::codem::Series &cdf_to_ultimate);

} // namespace pluss::subsp::actuarial::actuarial_models
