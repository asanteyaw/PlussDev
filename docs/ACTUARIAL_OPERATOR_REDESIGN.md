# PlussMa Actuarial Operator Redesign

## Objective

Replace the current catalogue of fixed actuarial formulas with a compact set of
domain values and typed operators. Existing vectorized formulas remain useful as
lower kernels, but the public surface should express assumptions, contingent
cash flows, insurance transformations and reserving estimators directly.

## Reference Implementations

The design was benchmarked against:

- `lifelib`, which treats actuarial work as inspectable models and formula
  dependency graphs suitable for pricing, valuation, projections and ALM.
- `actuarialmath`, whose hierarchy separates mortality assumptions, life tables,
  contracts, fractional-payment adjustments and valuation methods.
- CRAN `lifecontingencies`, which centers life-table and actuarial-table objects
  and uses a generic present-value kernel beneath product conveniences.
- CAS `chainladder-python`, which makes the claims triangle a first-class value
  and applies reusable development and reserving estimators to it.

Pluss should adopt those abstraction boundaries, not their language-specific
container models. Numerical work remains vectorized over `pluss::Series` and
must preserve platform execution.

## Public Core

The initial life-valuation core contains:

- `Mortality`: canonical finite-horizon mortality assumption.
- `Discount`: flat or explicitly supplied discount factors.
- `Basis`: mortality and discount assumptions used together.
- `Cashflow`: survival-contingent payments at times `0..n` and death-contingent
  payments at the ends of years `1..n`.
- `Operator<Signature>`: reusable, type-safe, type-erased domain operator.
- `Valuation`: the concise alias for `Operator<Series(const Cashflow&)>`.

The valuation identity is

```text
EPV = sum_t survival_payment[t] * Pr(T > t) * discount[t]
    + sum_k death_payment[k] * Pr(T > k) * q[k] * discount[k + 1].
```

Annuities, term insurance, pure endowments and endowment insurance are cash-flow
factories over this identity, not separate valuation algorithms.

## Phases

1. **Life valuation foundation**
   Implement mortality, discount, cash-flow and valuation operators. Rewire the
   existing life-contingency conveniences to the shared valuation kernel.
2. **Mortality generalization**
   Add select/ultimate, table-backed and user-defined mortality sources while
   retaining one canonical `Mortality` result.
   Parametric laws are reusable `MortalityModel` operators from attained-age
   `Series` values to that canonical result. Probability, force and lives-table
   inputs are constructors, not separate valuation branches.
3. **Loss operators**
   Represent frequency/severity as a `LossModel`; make deductibles, limits,
   coinsurance and layers reusable operators over empirical and modeled losses.
   Existing distribution cumulants and CDFs remain the lower statistical
   engines; policy transforms do not reimplement distributions.
4. **Pricing operators**
   Represent experience and expense assumptions explicitly. Credibility and
   loading become composable operators rather than formula-specific functions.
   A pricing run returns the pure-premium curve, loaded indication and rate
   change together so callers do not repeat the workflow by hand.
5. **Reserving operators**
   Introduce a labeled `Triangle` and reusable chain-ladder,
   Bornhuetter-Ferguson and expected-loss estimators with explicit result types.
6. **Migration and documentation**
   Remove redundant public formulas after all conveniences delegate to the new
   core, then rewrite the guides around the stable operator surface.

## Implementation Status

- Life valuation foundation: implemented; legacy annuity and insurance
  conveniences delegate to the shared valuation operator.
- Mortality generalization: implemented for qx, px, force, lives tables,
  select/ultimate paths and reusable Gompertz/Makeham models.
- Loss transformations: implemented for deductibles, limits, coinsurance,
  layers and empirical means; legacy empirical loss functions delegate.
- Pricing operators: implemented for experience rating, loads and credibility;
  the legacy rate-change workflow delegates.
- Reserving operators: implemented with a labeled `Triangle`, explicit
  observation masks, and reusable chain-ladder and Bornhuetter-Ferguson
  estimators. Explicit masks distinguish observed zeros from missing cells.
- Documentation migration: pending the final public-surface cleanup.

## Non-goals

- Do not reuse calculus `operators::Operator`; it is correctly specialized for
  `math::Function -> math::Function` structural transformations.
- Do not create one class per actuarial formula.
- Do not materialize host containers in execution kernels.
- Do not bind the design to only flat interest, scalar benefits, Poisson
  frequency, lognormal severity or one reserving method.
