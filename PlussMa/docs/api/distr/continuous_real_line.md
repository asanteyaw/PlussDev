<!-- generated from continuous_real_line.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# distr continuous, whole real line

Families whose values range over the whole real line: the normal and its heavy-tailed and skewed relatives, and the mixtures built from them. Each page documents one family: the parameters, the support, how to evaluate the density and the distribution function, how to draw samples, and, for the families that have one, the object form and the maximum-likelihood fit.

## The Surface

| Family | What it is |
|---|---|
| [`Cauchy`](continuous_real_line/Cauchy.md) | The Cauchy distribution: a symmetric bell-shaped curve with tails so heavy that it has no mean and no variance. |
| [`ExtremeValue`](continuous_real_line/ExtremeValue.md) | The extreme value (Gumbel) distribution: the distribution of the largest of many independent observations of a variable with an exponential-type tail. |
| [`Gh`](continuous_real_line/Gh.md) | The generalised hyperbolic distribution: a five-parameter family that contains the normal inverse Gaussian, the hyperbolic, the variance-gamma and, in a limit, the Student t. |
| [`HyperbolicSecant`](continuous_real_line/HyperbolicSecant.md) | The hyperbolic secant distribution: a symmetric density proportional to the hyperbolic secant, between the normal and the logistic in tail weight. |
| [`Laplace`](continuous_real_line/Laplace.md) | The Laplace (double-exponential) distribution: a symmetric peak with exponentially decaying tails, the distribution of the error whose best estimate is the median. |
| [`Logistic`](continuous_real_line/Logistic.md) | The logistic distribution: a bell-shaped curve slightly heavier-tailed than the normal, whose distribution function is the logistic (sigmoid) function. |
| [`Nig`](continuous_real_line/Nig.md) | The normal inverse Gaussian distribution: a four-parameter family for skewed, heavy-tailed data. |
| [`Normal`](continuous_real_line/Normal.md) | The normal (Gaussian) distribution: the bell curve, determined by its mean and standard deviation. |
| [`StudentT`](continuous_real_line/StudentT.md) | The Student t distribution: a bell-shaped curve with heavier tails than the normal, controlled by its degrees of freedom. |
| [`TukeyLambda`](continuous_real_line/TukeyLambda.md) | The Tukey lambda distribution: a one-parameter family defined through its quantile function that contains the logistic, uniform and approximately the normal and heavy-tailed distributions. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

continuous_real_line/Cauchy
continuous_real_line/ExtremeValue
continuous_real_line/Gh
continuous_real_line/HyperbolicSecant
continuous_real_line/Laplace
continuous_real_line/Logistic
continuous_real_line/Nig
continuous_real_line/Normal
continuous_real_line/StudentT
continuous_real_line/TukeyLambda
```
<!-- apiref:toctree:end -->
