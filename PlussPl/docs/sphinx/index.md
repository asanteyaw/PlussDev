<div id="pluss-landing">
<div class="pluss-hero">
<div class="kicker">Pluss &middot; Space</div>
<h1>PlussPl</h1>
<p class="tagline">The plotting and visualization layer of Pluss: a Studio owns Panes, each holding Traces drawn by the flash:: convenience layer, with a Theme/PaletteRamp/LightRig surface controlling how it all renders.</p>
<div class="pluss-hero-actions">
<a class="primary" href="api_reference.html">API Reference</a>
</div>
</div>

<div class="pluss-section">
<h2>What's in this space</h2>
<p class="section-sub">PlussPl is organized around three core object-model classes and the flash:: convenience layer built on top of them.</p>
<div class="pluss-package-grid">

<div class="pluss-card">
<span class="card-name">Studio, Pane, Trace</span>
<span class="card-model">Core Object Model</span>
<p class="card-desc">The figure (Studio), its plotting areas (Pane), and the individual drawn series/elements inside each pane (Trace).</p>
</div>

<div class="pluss-card">
<span class="card-name">flash::</span>
<span class="card-model">High-level Convenience Layer</span>
<p class="card-desc">~65 functions covering relational, distribution, categorical, regression, matrix, and multivariate plots — the surface most users reach for directly.</p>
</div>

<div class="pluss-card">
<span class="card-name">Theme &amp; Styling</span>
<span class="card-model">Theme &middot; PaletteRamp &middot; LightRig</span>
<p class="card-desc">Colors, fonts, palettes, and 3D lighting configuration shared across a Studio's panes and traces.</p>
</div>

</div>
</div>
</div>

# API Reference

This set follows the same usage-first API pattern used across the other Pluss docs: practical signatures, short behavioral notes, runnable examples, and concrete output artifacts (plots/frames).

```{toctree}
:maxdepth: 2
:hidden:

api_reference
studio_api_reference
pane_api_reference
trace_api_reference
flash_api_reference
build_outputs
```

```{toctree}
:maxdepth: 2
:hidden:

_local/MANIFEST
```
