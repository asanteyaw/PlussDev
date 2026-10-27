<!-- generated from inference_01_means_and_variances.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Hypothesis Testing 1: Means and Variances

A hypothesis test asks whether the data are consistent with a stated claim, the null hypothesis, for instance that a mean equals a target. It computes a statistic whose distribution is known if the claim is true, and reports how unusual the observed value is: the p-value, the probability under the null hypothesis of a statistic at least as extreme. After this chapter you can test means (one sample, paired, two samples), a mean with known spread, and variances, choose one- or two-sided alternatives, and read the result.

Every program is complete and its output is the output of that program. The data are fill volumes (in cl) from two cup-filling machines, A (8 cups) and B (9 cups), with a target of 5.0. The programs print with `OutputEngine` and take the standard 5% level unless they say otherwise.

## The result

Every test returns a struct that extends `TestDecision`:

| Field | Meaning |
|---|---|
| `stat` | the test statistic |
| `pvalue` | the probability, if the null is true, of a statistic as extreme as the observed one |
| `critical` | the critical value of the statistic at level `alpha` |
| `reject` | true when `pvalue < alpha` |

and the quantities specific to the test (degrees of freedom, means, standard error, an interval, effective sample sizes). Missing and non-finite values are dropped; the `effective_n` fields say how many observations were used.

`TestOptions` has `alpha` (default 0.05), `tail` (`Both`, `Right` or `Left`) and `method` (how a p-value is computed where an exact form exists; Chapter 2).

| Test | Question | Distribution of the statistic |
|---|---|---|
| `ttest(x, mean0)` | is the mean of `x` equal to `mean0`? | Student t, n − 1 df |
| `ttest_paired(x, y)` | is the mean difference of matched pairs zero? | Student t, n − 1 df |
| `ttest2(x, y, options)` | do two independent samples have equal means? | Student t (pooled) or Welch's t with approximate df |
| `ztest(x, mean0, sigma)` | the same as `ttest`, with the standard deviation known | standard normal |
| `vartest(x, variance0)` | is the variance of `x` equal to `variance0`? | chi-squared, n − 1 df |
| `vartest2(x, y)`, `f_ratio(x, y)` | do two samples have equal variances? | F, (nx − 1, ny − 1) df |

## Programs

- [Tests of means](inference_01_means_and_variances/tests_of_means.md)
- [A known standard deviation, and tests of variances](inference_01_means_and_variances/a_known_standard_deviation_and_tests_of_variance.md)
- [One-sided alternatives and the level](inference_01_means_and_variances/one_sided_alternatives_and_the_level.md)

## Checklist

1. State the null hypothesis and the alternative (the tail) before computing anything.
2. Two independent samples: Welch's test unless equal variances are justified; matched samples: the paired test.
3. Read the interval as well as the p-value: it shows the size of the effect.
4. A large p-value means the data cannot distinguish the hypotheses, not that the null is true.
5. These tests assume (near) normal data; for small samples that are not, use the rank tests of Chapter 2.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

inference_01_means_and_variances/tests_of_means
inference_01_means_and_variances/a_known_standard_deviation_and_tests_of_variance
inference_01_means_and_variances/one_sided_alternatives_and_the_level
```
<!-- apiref:toctree:end -->
