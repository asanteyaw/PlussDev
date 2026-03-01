# randraw.m Parity Status

This file tracks replacement status for `PlussMa/Stats/distributions/randraw.m`.

## Implemented in `pluss::subsp::stat::distr`

- `normal`, `gaussian`, `gauss`, `norm`
- `lognorm`, `lognormal`, `cobbdouglas`, `antilognormal`
- `uniform`, `unif`
- `exp`, `exponential`
- `bernoulli`, `bern`
- `cauchy`, `lorentz`, `caushy`
- `laplace`, `doubleexponential`, `doubleexp`, `bilateralexponential`, `bilateralexp`
- `logistic`
- `geometric`, `geom`, `furry`
- `rayl`, `rayleigh`
- `weibull`, `wbl`

## Added beyond `randraw.m`

- `mixture_normal(weights, means, stddevs, sizes, cfg, seed, use_sobol)`
- `pluss::sobol(...)`
- `pluss::sobol_normal(...)`

## Not yet at full `randraw.m` parity

The following `randraw.m` families are still missing from the C++ replacement:

- `alpha`
- `anglit`
- `arcsin`
- `beta`, `powerfunction`, `powerfunc`
- `bessel`
- `binom`, `binomial`
- `bradford`
- `burr`, `fisk`
- `chi`
- `chisquare`, `chisq`, `chi2`
- `chisqnc`, `chisqnoncentral`, `chisqnoncentr`, `chi2noncentral`
- `cosine`
- `erlang`
- `extrval`, `extremevalue`, `extrvalue`, `gumbel`
- `f`, `fdistribution`, `fdistrib`, `fdistr`, `fdist`, `fdis`
- `fnc`, `fnoncentral`, `fnoncentr`
- `gamma`
- `gig`
- `gh`
- `gompertz`
- `halfcosine`, `hcosine`, `hcos`
- `hyperbolicsecant`, `hsecant`, `hsec`
- `hypergeom`, `hypergeometric`
- `ig`, `inversegauss`, `invgauss`
- `maxwell`
- `nakagamim`, `nakagami-m`, `nakagami`
- `negativebinomial`, `negbinomial`, `negbinom`, `pascal`
- `normaltrunc`, `normaltruncated`, `gausstrunc`
- `nig`
- `pareto`
- `pareto2`, `lomax`
- `planck`
- `poisson`, `po`
- `quadratic`, `quad`, `quadr`
- `rademacher`
- `rice`, `rician`
- `semicirc`, `semicircle`, `wigner`
- `skellam`
- `studentst`, `t`
- `tri`, `triangular`
- `tukeylambda`
- `u`, `ushape`
- `vonmises`
- `wald`
- `frechet`
- `yule`, `yulesimon`
- `zeta`, `zipf`

## Notes

- Current `weibull` support is not yet parameter-parity complete with the legacy Matlab implementation.
- `frechet` is intentionally still listed as missing because `randraw.m` aliases it through a location-parameter Weibull form that is not yet mirrored here.
