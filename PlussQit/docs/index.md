<div id="pluss-landing">
<div class="pluss-hero">
<div class="kicker">Pluss &middot; Space</div>
<h1>PlussQit</h1>
<p class="tagline">The core object layer of Pluss: <code>Series</code> for numeric and boolean values, <code>Note</code> for vectorized text, <code>Timeline</code> for instants, and <code>Duration</code> for elapsed time.</p>
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
<p class="card-desc">The tensor-style numeric and boolean object underneath every numeric <code>Slate</code> column.</p>
</div>

<div class="pluss-card">
<span class="card-name">Note</span>
<span class="card-model">Text</span>
<p class="card-desc">The vectorized text object underneath every string-like <code>Slate</code> column.</p>
</div>

<div class="pluss-card">
<span class="card-name">Timeline</span>
<span class="card-model">Instants</span>
<p class="card-desc">The temporal object for absolute instants, calendar projections, timezone work, and ordered-axis access through <code>index()</code>.</p>
</div>

<div class="pluss-card">
<span class="card-name">Duration</span>
<span class="card-model">Elapsed Time</span>
<p class="card-desc">The temporal object for signed elapsed quantities, fixed-width offsets, and differences between instants.</p>
</div>

</div>
</div>
</div>

```{toctree}
:maxdepth: 2
:hidden:

manual/series
manual/note
manual/timeline
api_reference
```

```{toctree}
:glob:
:maxdepth: 2
:hidden:

*
_local/MANIFEST
```
