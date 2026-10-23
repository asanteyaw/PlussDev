<!-- generated from inference_02_ranks_distributions_and_tables.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Hypothesis Testing 2: Ranks, Distributions and Tables

The tests of Chapter 1 assume normal data. This chapter covers tests that do not: rank and sign tests for locations, tests of a whole distribution, and tests for several groups and for tables of counts. It also explains when a p-value is exact and when it is a large-sample approximation.

Every program is complete and its output is the output of that program. The cup data of Chapter 1 are used again.

## Exact and approximate p-values

A rank statistic takes only finitely many values, so its distribution under the null hypothesis can be listed exactly. For large samples that is too much work and the normal distribution does as well. `TestOptions::method` chooses:

| `PValueMethod` | Meaning |
|---|---|
| `Auto` (default) | exact for small samples, approximate otherwise |
| `Exact` | always exact |
| `Approximate` | always the large-sample approximation |

The `Auto` thresholds: `signrank` is exact for at most 15 nonzero differences; `ranksum` when the smaller sample has fewer than 10 observations and the two together fewer than 20; the one-sample `kstest` up to 140 observations; `kstest2` when the product of the two sample sizes is below 10 000. Exact p-values use the actual ranks, ties included (tied observations share their average rank, and the exact distribution is of those ranks). The approximation uses a continuity correction and a tie adjustment. `reject` is always `pvalue < alpha`.

## Programs

- [Rank and sign tests](inference_02_ranks_distributions_and_tables/rank_and_sign_tests.md)
- [Tests of a distribution](inference_02_ranks_distributions_and_tables/tests_of_a_distribution.md)
- [Several groups, and tables](inference_02_ranks_distributions_and_tables/several_groups_and_tables.md)

## Checklist

1. Small or non-normal samples: rank tests; they lose little power when the data are normal.
2. Small samples get exact p-values by default; force `Exact` or `Approximate` only with a reason.
3. Fix the null distribution's parameters before a Kolmogorov–Smirnov test.
4. ANOVA says whether means differ, not which; compare pairs afterwards.
5. Check expected counts in a chi-squared test.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

inference_02_ranks_distributions_and_tables/rank_and_sign_tests
inference_02_ranks_distributions_and_tables/tests_of_a_distribution
inference_02_ranks_distributions_and_tables/several_groups_and_tables
```
<!-- apiref:toctree:end -->
