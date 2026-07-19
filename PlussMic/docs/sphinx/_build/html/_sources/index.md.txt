<div id="pluss-landing">
<div class="pluss-hero">
<div class="kicker">Pluss &middot; Space</div>
<h1>PlussMic</h1>
<p class="tagline">The data-analysis layer of Pluss: tabular data (<code>DataFrame</code>), typed columns (<code>Thread</code>), stacks of related frames (<code>Atlas</code>), plus aligned streaming containers and heterogeneous record bundles.</p>
<div class="pluss-hero-actions">
<a class="primary" href="api_reference.html">API Reference</a>
</div>
</div>

<div class="pluss-section">
<h2>What's in this space</h2>
<p class="section-sub">PlussMic's public surface is organized around three models, plus a plotting/display branch shared across them.</p>
<div class="pluss-package-grid">

<div class="pluss-card">
<span class="card-name">table</span>
<span class="card-model">DataFrame &middot; Thread &middot; Atlas</span>
<p class="card-desc">Named, mixed-type tabular analysis: <code>DataFrame</code> for 2D tables, <code>Thread</code> for one typed column, <code>Atlas</code> for a depth stack of frames.</p>
</div>

<div class="pluss-card">
<span class="card-name">kindle</span>
<span class="card-model">Stream &middot; Catalog &middot; Archive</span>
<p class="card-desc">Aligned stream containers: a single named <code>Stream</code>, a <code>Catalog</code> of streams, and an <code>Archive</code> stack of catalogs.</p>
</div>

<div class="pluss-card">
<span class="card-name">bundle</span>
<span class="card-model">Slot &middot; Registry &middot; Aisle</span>
<p class="card-desc">Heterogeneous record containers: a one-dimensional <code>Slot</code>, a <code>Registry</code> of named slots, and an <code>Aisle</code> stack of registries.</p>
</div>

<div class="pluss-card">
<span class="card-name">Plotting</span>
<span class="card-model">DataFramePlotAccessor &middot; DataFramePlot3DAccessor</span>
<p class="card-desc">Plotting and display-oriented surfaces attached to the table model, reached through <code>DataFrame::plot()</code> and <code>DataFrame::plot3d()</code>.</p>
</div>

</div>
</div>
</div>

## Recommended Start Order

If you are learning the table workflow first, read these in order:

1. [Quickstart](quickstart.md)
2. [DataFrame Guide](dataframe.md)
3. [Manual: DataFrame Workflows](manual/part05_dataframe_workflows.md)
4. [API Reference](api_reference.md)

```{toctree}
:maxdepth: 2
:hidden:

api_reference
quickstart
manual
dataframe
kindle
bundle
demo_catalog
build_outputs
```

```{toctree}
:glob:
:maxdepth: 2
:hidden:

_local/*/*
_local/*/*/*
_local/*/*/*/*
```
