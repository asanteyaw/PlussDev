<div id="pluss-landing">
<div class="pluss-hero">
<div class="kicker">Pluss &middot; Space</div>
<h1>PlussQit</h1>
<p class="tagline">The core object layer of Pluss: <code>Series</code> (the numeric/boolean tensor object), <code>Note</code> (the vectorized text object), and the <code>Timeline</code> / <code>TimeIndex</code> time objects that every other space's <code>Thread</code> column ultimately wraps.</p>
<div class="pluss-hero-actions">
<a class="primary" href="api_reference.html">API Reference</a>
</div>
</div>

<div class="pluss-section">
<h2>What's in this space</h2>
<p class="section-sub">PlussQit owns the four foundational objects every table and time-series operation in the rest of Pluss is built from.</p>
<div class="pluss-package-grid">

<div class="pluss-card">
<span class="card-name">Series</span>
<span class="card-model">Numeric &middot; Boolean</span>
<p class="card-desc">The tensor-style numeric and boolean object underneath every numeric <code>Thread</code> column.</p>
</div>

<div class="pluss-card">
<span class="card-name">Note</span>
<span class="card-model">Text</span>
<p class="card-desc">The vectorized text object underneath every string-like <code>Thread</code> column.</p>
</div>

<div class="pluss-card">
<span class="card-name">Timeline</span>
<span class="card-model">Time</span>
<p class="card-desc">A time-point object, one of the two temporal payload kinds a <code>Thread</code> can carry.</p>
</div>

<div class="pluss-card">
<span class="card-name">TimeIndex</span>
<span class="card-model">Time</span>
<p class="card-desc">The second temporal payload kind, typically used for case-axis time labels.</p>
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
