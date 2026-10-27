<!-- generated from object_based.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Object based distributions

The object surface has two layers. [`Distribution`](object_based/Distribution.md) accepts any `dst::Kind` and delegates evaluation and sampling to the complete functional catalog. Dedicated objects such as `distr::Normal{mean, stddev}` carry strongly named parameters and provide exact family-specific operations. Both layers expose density, distribution, random-draw and cumulant methods, with maximum-likelihood fitting where supported. Each page documents constructors, method contracts and runnable examples. The kind-based functions remain on the family pages.

## The Surface

| Object | What it is |
|---|---|
| [`Bernoulli`](object_based/Bernoulli.md) | The object form of the Bernoulli distribution: a single trial with two outcomes, success with probability p and failure otherwise. |
| [`Beta`](object_based/Beta.md) | The object form of the beta distribution: a flexible family on a bounded interval, the standard model of proportions and probabilities. |
| [`Binomial`](object_based/Binomial.md) | The object form of the binomial distribution: the number of successes in a fixed number of independent trials with the same success probability. |
| [`Burr`](object_based/Burr.md) | The object form of the Burr (type III) distribution: a flexible heavy-tailed distribution with two shape parameters, with exact cumulants below the tail index. |
| [`ChiSquare`](object_based/ChiSquare.md) | The object form of the chi-squared distribution: the distribution of a sum of squared independent standard normal variables. |
| [`Distribution`](object_based/Distribution.md) | The generic object for every `dst::Kind`; it delegates evaluation and sampling to the functional catalog and supplies numerical cumulants, cumulant generating functions and maximum-likelihood fitting. |
| [`Exponential`](object_based/Exponential.md) | The object form of the exponential distribution: the waiting time between independent events that occur at a constant rate. |
| [`FDistribution`](object_based/FDistribution.md) | The object form of the F distribution: the distribution of a ratio of two independent chi-squared variables, each divided by its degrees of freedom. |
| [`Gamma`](object_based/Gamma.md) | The object form of the gamma distribution: a flexible family on the positive axis, the sum of exponential waiting times and the parent of the chi-squared family. |
| [`GaussianMixture`](object_based/GaussianMixture.md) | A mixture of normal distributions: a weighted sum of normal densities, fitted by expectation-maximisation. It has only an object form. |
| [`Geometric`](object_based/Geometric.md) | The object form of the geometric distribution: the number of independent trials needed to obtain the first success. |
| [`Gh`](object_based/Gh.md) | The object form of the generalised hyperbolic distribution: a five-parameter family that contains the normal inverse Gaussian, the hyperbolic, the variance-gamma and, in a limit, the Student t. |
| [`InverseGaussian`](object_based/InverseGaussian.md) | The object form of the inverse Gaussian distribution: the distribution of the first time a Brownian motion with positive drift reaches a fixed level. |
| [`Laplace`](object_based/Laplace.md) | The object form of the Laplace (double-exponential) distribution: a symmetric peak with exponentially decaying tails, the distribution of the error whose best estimate is the median. |
| [`Logistic`](object_based/Logistic.md) | The object form of the logistic distribution: a bell-shaped curve slightly heavier-tailed than the normal, whose distribution function is the logistic (sigmoid) function. |
| [`Lognormal`](object_based/Lognormal.md) | The object form of the lognormal distribution: the distribution of a variable whose logarithm is normal, the standard model of multiplicative growth. |
| [`Lomax`](object_based/Lomax.md) | The object form of the Lomax (Pareto type II) distribution: a power law that starts at zero, with exact cumulants below the tail index. |
| [`NegativeBinomial`](object_based/NegativeBinomial.md) | The object form of the negative binomial distribution: the number of failures before the r-th success, and the standard model of over-dispersed counts. |
| [`Nig`](object_based/Nig.md) | The object form of the normal inverse Gaussian distribution: a four-parameter family for skewed, heavy-tailed data. |
| [`Normal`](object_based/Normal.md) | The object form of the normal (Gaussian) distribution: the bell curve, determined by its mean and standard deviation. |
| [`NormalTrunc`](object_based/NormalTrunc.md) | The object form of the truncated normal distribution: a normal distribution restricted to an interval. |
| [`Pareto`](object_based/Pareto.md) | The object form of the Pareto distribution: the power-law distribution of sizes above a minimum, with exact cumulants below the tail index and an exact fit. |
| [`Poisson`](object_based/Poisson.md) | The object form of the Poisson distribution: the number of events in a fixed interval when events occur independently at a constant average rate. |
| [`Rayleigh`](object_based/Rayleigh.md) | The object form of the Rayleigh distribution: the length of a two-dimensional vector whose components are independent zero-mean normals. |
| [`Skellam`](object_based/Skellam.md) | The object form of the Skellam distribution: the difference of two independent Poisson counts, a distribution on the integers that can be negative. |
| [`StudentT`](object_based/StudentT.md) | The object form of the Student t distribution: a bell-shaped curve with heavier tails than the normal, controlled by its degrees of freedom. |
| [`Uniform`](object_based/Uniform.md) | The object form of the uniform distribution: every value in an interval is equally likely. |
| [`Weibull`](object_based/Weibull.md) | The object form of the Weibull distribution: the standard model of lifetimes and failure times, with a hazard that can fall, stay constant or rise. |
| [`Yule`](object_based/Yule.md) | The object form of the Yule–Simon distribution: a discrete power law with exact cumulants below the tail index. |
| [`Zeta`](object_based/Zeta.md) | The object form of the zeta (Zipf) distribution: a discrete power law on the positive integers, with exact cumulants below the tail index. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

object_based/Bernoulli
object_based/Beta
object_based/Binomial
object_based/Burr
object_based/ChiSquare
object_based/Distribution
object_based/Exponential
object_based/FDistribution
object_based/Gamma
object_based/GaussianMixture
object_based/Geometric
object_based/Gh
object_based/InverseGaussian
object_based/Laplace
object_based/Logistic
object_based/Lognormal
object_based/Lomax
object_based/NegativeBinomial
object_based/Nig
object_based/Normal
object_based/NormalTrunc
object_based/Pareto
object_based/Poisson
object_based/Rayleigh
object_based/Skellam
object_based/StudentT
object_based/Uniform
object_based/Weibull
object_based/Yule
object_based/Zeta
```
<!-- apiref:toctree:end -->
