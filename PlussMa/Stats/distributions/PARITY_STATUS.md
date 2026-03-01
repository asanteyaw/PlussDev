# randraw.m Parity Status

This file tracks the retired replacement surface for the legacy `randraw.m`.

## Implemented in `pluss::subsp::stat::distr`

- `alpha`
- `anglit`
- `arcsin`
- `bessel`
- `normal`, `gaussian`, `gauss`, `norm`
- `lognorm`, `lognormal`, `cobbdouglas`, `antilognormal`
- `uniform`, `unif`
- `exp`, `exponential`
- `bernoulli`, `bern`
- `beta`, `powerfunction`, `powerfunc`
- `binom`, `binomial`
- `bradford`
- `burr`, `fisk`
- `cauchy`, `lorentz`, `caushy`
- `chi`
- `chisquare`, `chisq`, `chi2`
- `chisqnc`, `chisqnoncentral`, `chisqnoncentr`, `chi2noncentral`
- `cosine`
- `erlang`
- `extrval`, `extremevalue`, `extrvalue`, `gumbel`
- `f`, `fdistribution`, `fdistrib`, `fdistr`, `fdist`, `fdis`
- `fnc`, `fnoncentral`, `fnoncentr`
- `laplace`, `doubleexponential`, `doubleexp`, `bilateralexponential`, `bilateralexp`
- `logistic`
- `geometric`, `geom`, `furry`
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
- `rayl`, `rayleigh`
- `rice`, `rician`
- `semicirc`, `semicircle`, `wigner`
- `skellam`
- `studentst`, `t`
- `tri`, `triangular`
- `tukeylambda`
- `u`, `ushape`
- `vonmises`
- `wald`
- `weibull`, `wbl`, `frechet`
- `yule`, `yulesimon`
- `zeta`, `zipf`

## Added beyond `randraw.m`

- `mixture_normal(weights, means, stddevs, sizes, cfg, seed, use_sobol)`
- `pluss::sobol(...)`
- `pluss::sobol_normal(...)`

## Notes

- `randraw.m` can be retired. The C++ distribution surface now covers the full legacy catalog.
- The C++ surface now mirrors the legacy location-parameter Weibull form, so the `frechet` alias is covered.
