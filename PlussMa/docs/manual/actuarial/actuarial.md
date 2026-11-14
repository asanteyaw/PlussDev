<!-- generated from actuarial.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Actuarial mathematics

The actuarial surface is organized around four connected questions:

1. **How long does a life or risk survive?** Mortality probabilities, forces of mortality, survival paths and life tables establish the probability model.
2. **What is a contingent payment worth today?** Annuities, insurance, endowments, commutation columns, premiums and reserves combine mortality with discounting.
3. **How large can aggregate losses become?** Frequency and severity moments, compound models, deductibles, limits and layers describe insurance losses.
4. **What rate or reserve does experience indicate?** Credibility, expense loads, chain ladder and Bornhuetter–Ferguson turn observations into pricing and reserving decisions.

## One operator architecture

These questions share one design rather than four unrelated formula catalogues.
Domain values carry assumptions and observations; typed operators perform the
actuarial transformation:

- `Mortality`, `Discount`, `Basis` and `Cashflow` feed a reusable `Valuation`.
- `LossTransform` represents deductibles, limits, coinsurance and layers.
- `Experience` and `Loads` feed a `Pricing` operator and produce one
  `PricingResult` containing the pure premium, indicated premium and rate
  change.
- `Triangle` carries cumulative claims, an explicit observation mask and
  optional origin/development labels. A `Reserving` operator produces one
  `ReserveResult` containing development factors, ultimate losses and reserves.

This separation is important. A term-insurance benefit is a `Cashflow`, not a
new valuation algorithm. A 100-excess-of-500 layer is a reusable transform, not
a lognormal-only formula. Chain ladder is an estimator applied to a `Triangle`,
not a convention that every zero in an unlabeled matrix must mean “missing”.

The original formula functions remain available as concise conveniences, but
their implementations delegate to the shared operators wherever the contracts
match. The operator surface is the extensible route for non-uniform benefits,
yield curves, custom mortality models, policy terms, pricing workflows and
explicitly observed claims triangles.

Chapters, in reading order:

- [Actuarial 1: Mortality and Life Tables](actuarial_01_mortality_and_life_tables.md)
- [Actuarial 2: Life Contingencies, Premiums and Reserves](actuarial_02_life_contingencies_premiums_and_reserves.md)
- [Actuarial 3: Frequency, Severity and Insurance Layers](actuarial_03_frequency_severity_and_insurance_layers.md)
- [Actuarial 4: Ratemaking, Credibility and Loss Reserving](actuarial_04_ratemaking_credibility_and_loss_reserving.md)
- [Actuarial 5: Worked Problems](actuarial_05_worked_problems.md)

Every chapter follows the same format: compact mathematical definitions, explicit contracts, complete programs, and output produced by those programs.

```{toctree}
:maxdepth: 3
:hidden:

actuarial_01_mortality_and_life_tables
actuarial_02_life_contingencies_premiums_and_reserves
actuarial_03_frequency_severity_and_insurance_layers
actuarial_04_ratemaking_credibility_and_loss_reserving
actuarial_05_worked_problems
```
