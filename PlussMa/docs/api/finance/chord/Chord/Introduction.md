<!-- generated from Introduction.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `module::Chord` Introduction

A `Chord` holds assets in the order they were added, each a `Field` under a unique name. Every operation is the `Field`'s own operation applied to each asset, so an asset can have any model and engine the `Field` supports, and different assets can have different ones.

**Building.** A `Chord` is built from a dictionary of assets at construction (`Chord book{{"spx", spx}, {"ndx", ndx}}`, see [`Chord::Chord`](Chord.md)), by [`add`](add.md) one asset or a dictionary at a time, or, for assets of any of the six models whose parameters are numbers, from a table with [`from_model_params`](from_model_params.md). The order of the assets is the order of every result and of the correlation matrices.

**Batching.** Two assets are merged into one call when they have the same engine (`Analytic`, `Cos` or `Fft`), the same controls and risk setting, European exercise, and the same model family; their strips are laid end to end, priced once, and split back. Black-Scholes assets on the analytic engine and Heston and Bates assets on the COS engine merge **whatever their parameters, rates and spots**, because the closed form and the COS series take every parameter per option; calls and puts mix freely. FFT assets merge only when their model parameters and option type are also equal. The FFT with unequal parameters, the lattice, the PDE, the American engines and Monte Carlo with a control variate or importance sampling make one call per asset, since each builds its own grid or simulation; those calls run concurrently. Monte Carlo Heston and Bates assets with the same controls, a single maturity per asset and a scalar market share one simulation with an asset axis (the assets independent), which prices and gives Greeks for all of them together. [`batches`](batches.md) counts the calls a price would make. For the closed forms and the transform engines the merged result is the same as pricing each asset alone; for the shared Monte Carlo simulation it agrees up to Monte Carlo error, because the assets draw their random numbers from one stream.

**Correlation.** A `Chord` also carries the relationships between its assets: `price_corr` and `var_corr`, [assets, assets] correlation matrices (undefined means independent) of the price shocks and of the variance shocks, and `price_var_cross`, the cross correlation of one asset's price shock with another's variance shock, which is built from the other two and each asset's `rho` when it is not given. They are used by [`simulate_paths`](simulate_paths.md), which simulates all assets together in one pass, whatever their model (Black-Scholes, SABR, Heston, Bates, rough Heston and rough Bergomi can be mixed) so that payoffs on several assets (baskets, spreads) can be evaluated. Each asset's own law is unchanged by the correlation, so `price` of single-asset options does not depend on it.

**Concurrency.** The calls of a `price` are independent, so `price` runs them on several threads (up to one per core). The public member `parallel` (default true) switches that off; the results are identical either way, bit for bit, because each call writes only its own assets' results.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Chord.html">
    <span class="label">Next</span>
    <span class="title">Chord</span>
  </a>
</div>
<!-- apiref:nav:end -->
