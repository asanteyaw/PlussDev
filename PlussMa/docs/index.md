<div id="pluss-landing">
<div class="pluss-hero">
<div class="kicker">Pluss &middot; Space</div>
<h1>PlussMa</h1>
<p class="tagline">Numerical and mathematical foundations for Pluss: optimization routines, probability distributions, statistical finance, and regression, organized as one topic folder per domain.</p>
<div class="pluss-hero-actions">
<a class="primary" href="api_reference.html">API Reference</a>
</div>
</div>

<div class="pluss-section">
<h2>What's in this space</h2>
<p class="section-sub">Each domain below owns its own nested <code>docs/</code> folder next to its source code, aggregated into this hub automatically.</p>
<div class="pluss-package-grid">

<div class="pluss-card">
<span class="card-name">Optimization</span>
<span class="card-model">Math/Numerical/optimization</span>
<p class="card-desc">Optimizers, sequential QP, and the internal solver protocol.</p>
</div>

<div class="pluss-card">
<span class="card-name">Distributions</span>
<span class="card-model">Stats/distributions</span>
<p class="card-desc">Probability distribution objects and their reference API.</p>
</div>

<div class="pluss-card">
<span class="card-name">Finance &amp; Regression</span>
<span class="card-model">Stats/finance, Stats/regression</span>
<p class="card-desc">Statistical finance and regression tooling, as those topic folders grow their own <code>docs/</code>.</p>
</div>

</div>
</div>
</div>

```{toctree}
:maxdepth: 2
:hidden:

api_reference
```

```{toctree}
:glob:
:maxdepth: 2
:hidden:

*
_local/*/*
_local/*/*/*
_local/*/*/*/*
```
