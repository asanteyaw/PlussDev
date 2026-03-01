# Distribution Catalog

The catalog below follows the retired `randraw.m` naming scheme so older usage strings still map cleanly into the C++ layer.

## Core Continuous Families

- `normal`, `gaussian`, `gauss`, `norm`
- `lognorm`, `lognormal`, `cobbdouglas`, `antilognormal`
- `uniform`, `unif`
- `exp`, `exponential`
- `cauchy`, `lorentz`, `caushy`
- `laplace`, `doubleexponential`, `doubleexp`, `bilateralexponential`, `bilateralexp`
- `logistic`
- `studentst`, `t`
- `normaltrunc`, `normaltruncated`, `gausstrunc`
- `vonmises`

## Shape / Tail Families

- `beta`, `powerfunction`, `powerfunc`
- `gamma`
- `weibull`, `wbl`, `frechet`
- `pareto`
- `pareto2`, `lomax`
- `burr`, `fisk`
- `gompertz`
- `planck`
- `gig`
- `gh`
- `nig`

## Symmetric / Bounded Families

- `alpha`
- `anglit`
- `arcsin`
- `cosine`
- `halfcosine`, `hcosine`, `hcos`
- `hyperbolicsecant`, `hsecant`, `hsec`
- `quadratic`, `quad`, `quadr`
- `tri`, `triangular`
- `u`, `ushape`
- `semicirc`, `semicircle`, `wigner`

## Count / Discrete Families

- `bernoulli`, `bern`
- `binom`, `binomial`
- `geom`, `geometric`, `furry`
- `poisson`, `po`
- `negativebinomial`, `negbinomial`, `negbinom`, `pascal`
- `hypergeom`, `hypergeometric`
- `bessel`
- `chisquare`, `chisq`, `chi2`
- `chisqnc`, `chisqnoncentral`, `chisqnoncentr`, `chi2noncentral`
- `skellam`
- `rademacher`
- `yule`, `yulesimon`
- `zeta`, `zipf`

## Derived / Physics / Special Cases

- `chi`
- `erlang`
- `f`, `fdistribution`, `fdistrib`, `fdistr`, `fdist`, `fdis`
- `fnc`, `fnoncentral`, `fnoncentr`
- `ig`, `inversegauss`, `invgauss`
- `wald`
- `rayl`, `rayleigh`
- `maxwell`
- `nakagamim`, `nakagami-m`, `nakagami`
- `rice`, `rician`

## Parameter Conventions

Selected conventions carried over from the legacy surface:

- `beta`: `[a, b]` or `[m, n, a, b]`
- `gamma`: `[a]` or `[m, b, a]`
- `t`: `[nu]` or `[nu, chi, eta]`
- `chi`: `[nu]` or `[a, b, nu]`
- `weibull`: `[shape, scale]` or legacy `[theta, alpha, beta]`
- `normaltrunc`: `[a, b, mu, sigma]`
- `hypergeom`: `[N, M, n]`
- `nig`: `[alpha, beta, mu, delta]`
- `gh`: `[lambda, alpha, beta, mu, delta]`
- `gig`: `[lambda, chi, psi]`

When in doubt, use `pluss::distr::supports(name)` first and then keep the parameter list minimal.
