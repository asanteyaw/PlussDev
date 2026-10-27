<!-- generated from groups_and_tables.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# infer: groups and tables

Analysis of variance for several groups, and chi-squared tests for counts and tables. Each page documents one function: its signature, what it computes, the parameters, the fields of the result, the errors, and examples with their output.

## The Surface

| Function | What it does |
|---|---|
| [`anova_1w`](groups_and_tables/anova_1w.md) | One-way analysis of variance: do several group means agree? |
| [`chisq_gof`](groups_and_tables/chisq_gof.md) | Chi-squared goodness-of-fit test of observed counts against expected counts. |
| [`chisq_indep`](groups_and_tables/chisq_indep.md) | Chi-squared test of independence for a table of counts. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

groups_and_tables/anova_1w
groups_and_tables/chisq_gof
groups_and_tables/chisq_indep
```
<!-- apiref:toctree:end -->
