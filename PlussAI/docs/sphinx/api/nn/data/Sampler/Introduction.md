# `pluss::loader::Sampler` Introduction

`Sampler` owns exactly one decision: given a dataset of `size` samples
and a seed for the current epoch, what order does
[`DataLoader`](../DataLoader.md) walk them in. It exists as a pluggable
alternative to `DataLoaderOptions::shuffle`'s plain bool, for the cases a
bool cannot express -- training on a fixed subset every epoch
([`SubsetRandomSampler`](../SubsetRandomSampler.md)), oversampling a
minority class ([`WeightedRandomSampler`](../WeightedRandomSampler.md)),
or a fully custom strategy via a subclass of `Sampler` itself.

`sample()` returns the *entire* epoch's order eagerly, as one
`std::vector<std::int64_t>`, rather than a lazy per-element iterator --
matching the eager `order_` vector `DataLoader` already builds internally
for its own plain `shuffle` path, and avoiding a coroutine/generator
design this codebase has no existing idiom for.

`sampler` and `shuffle` are mutually exclusive on
[`DataLoaderOptions`](../DataLoaderOptions.md) -- setting both throws at
`DataLoader` construction.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="sample.html">
    <span class="label">Next</span>
    <span class="title">sample</span>
  </a>
</div>
<!-- apiref:nav:end -->
