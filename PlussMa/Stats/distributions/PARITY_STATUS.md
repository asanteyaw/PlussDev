# randraw.m Parity Status

This file tracks replacement status for `PlussMa/Stats/distributions/randraw.m`.

## Implemented in `pluss::subsp::stat::distr`

- `normal`, `gaussian`, `gauss`, `norm`
- `lognorm`, `lognormal`, `cobbdouglas`, `antilognormal`
- `uniform`, `unif`
- `exp`, `exponential`
- `bernoulli`, `bern`
- `beta`, `powerfunction`, `powerfunc`
- `binom`, `binomial`
- `cauchy`, `lorentz`, `caushy`
- `chisquare`, `chisq`, `chi2`
- `laplace`, `doubleexponential`, `doubleexp`, `bilateralexponential`, `bilateralexp`
- `logistic`
- `geometric`, `geom`, `furry`
- `gamma`
- `gig`
- `gh`
- `hypergeom`, `hypergeometric`
- `ig`, `inversegauss`, `invgauss`
- `normaltrunc`, `normaltruncated`, `gausstrunc`
- `nig`
- `pareto`
- `pareto2`, `lomax`
- `poisson`, `po`
- `rayl`, `rayleigh`
- `skellam`
- `studentst`, `t`
- `tri`, `triangular`
- `wald`
- `weibull`, `wbl`
- `zeta`, `zipf`

## Added beyond `randraw.m`

- `mixture_normal(weights, means, stddevs, sizes, cfg, seed, use_sobol)`
- `pluss::sobol(...)`
- `pluss::sobol_normal(...)`

## Still missing from `randraw.m` parity

The following `randraw.m` families are still Matlab-only:

- `alpha`
- `anglit`
- `arcsin`
- `bessel`
- `bradford`
- `burr`, `fisk`
- `chi`
- `chisqnc`, `chisqnoncentral`, `chisqnoncentr`, `chi2noncentral`
- `cosine`
- `erlang`
- `extrval`, `extremevalue`, `extrvalue`, `gumbel`
- `f`, `fdistribution`, `fdistrib`, `fdistr`, `fdist`, `fdis`
- `fnc`, `fnoncentral`, `fnoncentr`
- `gompertz`
- `halfcosine`, `hcosine`, `hcos`
- `hyperbolicsecant`, `hsecant`, `hsec`
- `maxwell`
- `nakagamim`, `nakagami-m`, `nakagami`
- `negativebinomial`, `negbinomial`, `negbinom`, `pascal`
- `planck`
- `quadratic`, `quad`, `quadr`
- `rademacher`
- `rice`, `rician`
- `semicirc`, `semicircle`, `wigner`
- `tukeylambda`
- `u`, `ushape`
- `vonmises`
- `frechet`
- `yule`, `yulesimon`

## Notes

- This C++ surface now covers the three planned parity batches: high-value core families, transform-based continuous families, and the heavy-tail/special-function batch (`hypergeom`, `skellam`, `zipf`/`zeta`, `gig`, `gh`, `nig`).
- Full replacement is still not complete. `randraw.m` should not be deleted until the remaining Matlab-only families above are implemented or intentionally dropped.
- Current `weibull` support is still not parameter-parity complete with the legacy Matlab implementation.
- `frechet` remains missing because `randraw.m` aliases it through a location-parameter Weibull form that is not yet mirrored here.
