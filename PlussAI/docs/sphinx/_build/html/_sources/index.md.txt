# PlussAI

<div id="pluss-landing">
<div class="pluss-hero">
<div class="kicker">Pluss &middot; Space</div>
<h1>PlussAI</h1>
<p class="tagline">The machine-learning layer of Pluss: <code>classic</code> for classical ML, <code>nets</code> for neural modules, <code>nlp</code> for text workflows, <code>lymph</code> for high-level modeling, <code>ice</code> for training/runtime orchestration, and <code>procedure</code> for artifact-driven execution.</p>
<div class="pluss-hero-actions">
<a class="primary" href="api_reference.html">API Reference</a>
</div>
</div>

<div class="pluss-section">
<h2>What's in this space</h2>
<p class="section-sub">PlussAI's public surface is organized by family, with human-facing namespaces layered over the internal implementation spaces.</p>
<div class="pluss-package-grid">

<div class="pluss-card">
<span class="card-name">classic</span>
<span class="card-model">Classical ML Core</span>
<p class="card-desc">The public classical-ML namespace over the internal NodeJ substrate: batches, splits, prep, metrics, families, tuning, and persistence contracts.</p>
</div>

<div class="pluss-card">
<span class="card-name">nets</span>
<span class="card-model">Neural Modules</span>
<p class="card-desc">The neural/module surface over the internal NodeK implementation: modules, losses, embeddings, RNNs, attention, convolution, and optimization helpers.</p>
</div>

<div class="pluss-card">
<span class="card-name">nlp</span>
<span class="card-model">Text &amp; Language</span>
<p class="card-desc">The public NLP namespace over the internal NodeB layer: docs, tokenization, vectorization, embeddings, text runtimes, and NLP workflows.</p>
</div>

<div class="pluss-card">
<span class="card-name">lymph</span>
<span class="card-model">Model Workspace</span>
<p class="card-desc">High-level modeling, validation, selection, spotchecking, workflow composition, and convenience wrappers over the classical ML core.</p>
</div>

<div class="pluss-card">
<span class="card-name">ice</span>
<span class="card-model">Training &amp; Runtime</span>
<p class="card-desc">Training loops, sources, coaches, outlines, model orchestration, and deep-learning convenience surfaces over the neural family.</p>
</div>

<div class="pluss-card">
<span class="card-name">procedure</span>
<span class="card-model">Execution &amp; Persistence</span>
<p class="card-desc">Artifact-driven staged execution, family-aware persistence, and reusable execution graphs across PlussAI components.</p>
</div>

</div>
</div>
</div>

## API Reference

This set follows a reference-style API pattern: signature + parameter-by-parameter reference, return semantics and behavior notes, usage snippets, and verified output blocks captured from executed demos.

```{toctree}
:maxdepth: 2
:hidden:

api_reference
build_outputs
```

```{toctree}
:maxdepth: 1
:hidden:

_local/MANIFEST
```
