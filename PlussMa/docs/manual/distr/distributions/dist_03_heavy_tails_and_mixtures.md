<!-- generated from dist_03_heavy_tails_and_mixtures.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Distributions 3: Heavy Tails and Mixtures

The normal distribution is a poor model for many real series: financial returns, insurance losses and measurement errors have heavier tails and skew. Two remedies are provided: a family with tail weight and skewness parameters of its own, the normal inverse Gaussian (and its generalisation, the generalised hyperbolic), and mixtures of normals, which build multimodal or skewed shapes from simple parts. After this chapter you can fit both by maximum likelihood, compare them with the normal, and read their standard errors.

Every program is complete and its output is the output of that program.

## The normal inverse Gaussian and generalised hyperbolic families

The NIG density with parameters `alpha > |beta|`, location `μ` and scale `δ` is

> f(x) = α δ K₁(α q) / (π q) · exp(δ γ + β (x − μ)),    q = √(δ² + (x − μ)²),  γ = √(α² − β²),

with `K₁` the modified Bessel function of the second kind. `alpha` governs the tails (the larger, the lighter; the tails decay like `exp(−(α − β)x)`), `beta` the skewness (negative: a longer left tail), `location` and `scale` the centre and spread. The generalised hyperbolic family `Gh{lambda, alpha, beta, location, scale}` adds the order `λ` of the Bessel function in the normalising constant: `λ = −1/2` is exactly the NIG, `λ = 1` the hyperbolic distribution. Both are objects with the methods of Chapter 1 (and `sample`, `cgf`, `cumulant`) and the fit methods of Chapter 2.

`Nig::MLE(x)` maximises the exact log-likelihood over all four parameters from several starting shapes and keeps the best. The likelihood of `Gh::MLE(x)` is flat along a ridge in the parameters, and the maximum may lie on the boundary where the scale `δ` shrinks to zero (a variance-gamma limit that is not itself a member of the family). The fit keeps `δ` above one thousandth of the sample standard deviation; if the estimate is at that bound the sample does not distinguish the family from its limit, and the intervals should not be trusted. Fixing `λ` by using the NIG is the usual remedy.

## Mixtures of normals

`GaussianMixture{weights, means, stddevs}` is the density `Σ_j w_j N(x; m_j, s_j²)`. `MLE(x)` fits the weights, means and standard deviations by the expectation-maximisation algorithm: it starts from the values stored in the object (their number is the number of components), alternates between computing each observation's component probabilities and re-estimating the parameters, and stops when the log-likelihood no longer changes (a relative change below 10⁻¹³). Different starting values reach the same maximum unless the likelihood has several local ones, so for a real problem try a few starts and keep the highest log-likelihood.

`stderr`, `vcov` and `confint` of a fitted mixture are for the natural parameters, in the order weights (`k`), means (`k`), standard deviations (`k`). They are computed from the Hessian of the log-likelihood in unconstrained coordinates (the logits of the first `k − 1` weights relative to the last, the means, the logarithms of the standard deviations) by the delta method, which respects that the weights sum to one: the weights have equal standard errors for two components and their covariance matrix is singular. `score` and `observed_inf` are in the unconstrained coordinates.

## Programs

- [Heavy tails](dist_03_heavy_tails_and_mixtures/heavy_tails.md)
- [Mixtures](dist_03_heavy_tails_and_mixtures/mixtures.md)

## Checklist

1. Compare heavy-tailed fits with the normal by log-likelihood and Akaike's criterion, and look at the probability of the tail event that matters to you.
2. Prefer the NIG to the generalised hyperbolic family unless the sample is large; treat a `Gh` scale at its lower bound as a sign that the extra parameter is not identified.
3. For mixtures, choose the number of components by an information criterion and try several starting values.
4. Read the weights' standard errors with their constraint in mind: they sum to one.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

dist_03_heavy_tails_and_mixtures/heavy_tails
dist_03_heavy_tails_and_mixtures/mixtures
```
<!-- apiref:toctree:end -->
