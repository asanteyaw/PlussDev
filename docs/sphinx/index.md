<div id="pluss-landing">
<div class="pluss-hero">
<div class="kicker">Pluss Documentation</div>
<h1>Pluss</h1>
<p class="tagline">A C++20 technical-computing ecosystem for tables, arrays, time series, plotting, statistics, optimization, and machine learning &mdash; built as six focused packages that compose into one stack.</p>
<div class="pluss-hero-actions">
<a class="primary" href="installation.html">Get Started</a>
<a class="secondary" href="quickstart.html">Quickstart</a>
<a class="secondary" href="user_guide.html">User Guide</a>
</div>
<div class="pluss-hero-include">#include &lt;pluss/pluss.h&gt;</div>
</div>

<div class="pluss-section">
<h2>Explore the ecosystem</h2>
<p class="section-sub">Six spaces, each covering one part of the stack. Open a space's API Reference to see its objects and methods in full.</p>
<div class="pluss-package-grid">

<div class="pluss-card">
<span class="card-name">PlussMic</span>
<span class="card-model">DataFrame &middot; Thread &middot; Atlas</span>
<p class="card-desc">The data-analysis layer: tabular data (<code>DataFrame</code>), typed columns (<code>Thread</code>), stacks of related frames (<code>Atlas</code>), plus streaming, bundling, and plotting.</p>
<div class="card-links">
<a href="_local/PlussMic/api_reference.html">API Reference &rarr;</a>
</div>
</div>

<div class="pluss-card">
<span class="card-name">PlussQit</span>
<span class="card-model">Series &middot; Note &middot; Timeline &middot; TimeIndex</span>
<p class="card-desc">The core object layer: the numeric/boolean tensor object (<code>Series</code>), the vectorized text object (<code>Note</code>), and the two time objects that <code>Thread</code> builds on.</p>
<div class="card-links">
<a href="_local/PlussQit/api_reference.html">API Reference &rarr;</a>
</div>
</div>

<div class="pluss-card">
<span class="card-name">PlussMa</span>
<span class="card-model">Optimization &middot; Distributions &middot; Finance</span>
<p class="card-desc">Numerical and mathematical foundations: optimization routines, probability distributions, statistical finance, and regression.</p>
<div class="card-links">
<a href="_local/PlussMa/api_reference.html">API Reference &rarr;</a>
</div>
</div>

<div class="pluss-card">
<span class="card-name">PlussPl</span>
<span class="card-model">Flash &middot; Plotting</span>
<p class="card-desc">The plotting and visualization layer, including the Flash scene, theme, and animation surface used to render tables and series to screen.</p>
<div class="card-links">
<a href="_local/PlussPl/api_reference.html">API Reference &rarr;</a>
</div>
</div>

<div class="pluss-card">
<span class="card-name">PlussAI</span>
<span class="card-model">NodeJ &middot; Lymph</span>
<p class="card-desc">The machine-learning and graph-computation layer, covering the NodeJ differentiable-computation surface and the Lymph model layer.</p>
<div class="card-links">
<a href="_local/PlussAI/api_reference.html">API Reference &rarr;</a>
</div>
</div>

<div class="pluss-card">
<span class="card-name">PlussTils</span>
<span class="card-model">Pathing &middot; View &middot; Aux</span>
<p class="card-desc">Cross-cutting utilities shared by every other space: filesystem pathing, the PlussView desktop viewer, and auxiliary helpers such as <code>OutputEngine</code>.</p>
<div class="card-links">
<a href="_local/PlussTils/api_reference.html">API Reference &rarr;</a>
</div>
</div>

</div>
</div>

<div class="pluss-section">
<h2>Core mental model</h2>
<p class="section-sub">Pluss is organized around a small set of composable data objects. Every package above ultimately builds on, or feeds, this stack.</p>
<div class="pluss-model-diagram">Atlas -&gt; DataFrame -&gt; Thread -&gt; Series / Note / Timeline / TimeIndex</div>
<ul class="pluss-model-list">
<li><code>Series</code> is the numeric and boolean tensor-style object.</li>
<li><code>Note</code> is the vectorized text object.</li>
<li><code>Timeline</code> and <code>TimeIndex</code> are time objects.</li>
<li><code>Thread</code> is a typed table column over one of those core objects.</li>
<li><code>DataFrame</code> is a 2D table of named <code>Thread</code> features over a case axis.</li>
<li><code>Atlas</code> is a stack of related <code>DataFrame</code> objects.</li>
</ul>
</div>

<div class="pluss-section">
<h2>What to read first</h2>
<p class="section-sub">If you are new to Pluss, read these in order. After that, use the package cards above and the topic guides as reference material.</p>
<ol class="pluss-start-steps">
<li><a href="installation.html">Installation and CMake</a><p>Build the SDK and consume it from another project.</p></li>
<li><a href="quickstart.html">Quickstart</a><p>Build a first program and meet the main objects.</p></li>
<li><a href="user_guide.html">User Guide</a><p>The conceptual map of the ecosystem.</p></li>
<li><a href="data_workflows.html">Data Workflows</a><p>Loading, inspecting, filtering, deriving, grouping, plotting, saving.</p></li>
<li><a href="numerical_workflows.html">Numerical Workflows</a><p><code>Series</code>, math, distributions, optimization, differentiation.</p></li>
</ol>
</div>
</div>

The most common CMake target for downstream projects:

```cmake
find_package(Pluss CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE Pluss::SDK)
```

```{toctree}
:maxdepth: 2
:hidden:

installation
quickstart
user_guide
data_workflows
numerical_workflows
lymph_user_guide
lymph_cookbook
lymph_combination_atlas
lymph_troubleshooting
nodej_user_guide
nodej_cookbook
plusspl_user_guide
plusspl_api_reference
flash_user_guide
flash_api_reference
plussma_optimization_user_guide
plussma_optimization_cookbook
plussma_distributions_user_guide
plussma_distributions_cookbook
plussma_finance_user_guide
plussma_finance_cookbook
plussma_finance_option_atlas
plussma_finance_troubleshooting
plussma_regression_user_guide
plussma_regression_api_reference
plussma_time_series_user_guide
plussma_hypothesis_user_guide
demo_catalog
plussmic_docs_hub
time_series_accessor_guide
series
series_query_user_guide
aux
aux_tutorial
tils_env
loader
io_save_load_guide
error_handling
autograd
autograd_api_reference
autodiff_architecture
optimization_diff_contract
dl_torch_maturity_blueprint
series_api_reference
series_all_methods_usage
note
content_moment_helpers
note_api_reference
note_all_methods_usage
timeline_timeindex
timeline_api_reference
timeindex_api_reference
timeline_all_methods_usage
timeindex_all_methods_usage
api_indexes
build_outputs
README
```

```{toctree}
:glob:
:maxdepth: 2
:hidden:

_local/*/*
_local/*/*/*
_local/*/*/*/*
```
