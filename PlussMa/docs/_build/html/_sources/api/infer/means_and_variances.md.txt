<!-- generated from means_and_variances.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# infer: means and variances

Tests of means (one sample, paired, two samples), of a mean with known spread, and of variances. Each page documents one function: its signature, what it computes, the parameters, the fields of the result, the errors, and examples with their output.

## The Surface

| Function | What it does |
|---|---|
| [`ttest`](means_and_variances/ttest.md) | One-sample Student t-test of a mean. |
| [`ttest_paired`](means_and_variances/ttest_paired.md) | Paired-sample t-test: is the mean difference of matched pairs zero? |
| [`ttest2`](means_and_variances/ttest2.md) | Two-sample t-test for the means of two independent samples, pooled or Welch. |
| [`ztest`](means_and_variances/ztest.md) | One-sample z-test of a mean when the standard deviation is known. |
| [`vartest`](means_and_variances/vartest.md) | Chi-squared test of one variance against a stated value. |
| [`vartest2`](means_and_variances/vartest2.md) | F test for the equality of two variances. |
| [`f_ratio`](means_and_variances/f_ratio.md) | The F test for two variances, under its other name. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

means_and_variances/ttest
means_and_variances/ttest_paired
means_and_variances/ttest2
means_and_variances/ztest
means_and_variances/vartest
means_and_variances/vartest2
means_and_variances/f_ratio
```
<!-- apiref:toctree:end -->
