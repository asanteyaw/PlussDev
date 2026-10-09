<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: a7a7cec99bba -->
# `pluss::ai::nlp::embed::Embedder::load`

[Back to `Embedder`](../Embedder.md)

## Signature

```cpp
static pluss::ai::nlp::embed::Embedder
pluss::ai::nlp::embed::Embedder::load(std::string_view filepath);
```

## What It Does

`load()` constructs a ready model from a static-embedding artifact. It verifies the magic header, supported schema, dimensions, entry count, vector widths, and body checksum before publishing state. Version-two artifacts restore lemma, case, and punctuation policies as well as vectors; legacy version-one artifacts use the historical defaults for those policies.

The returned model owns all loaded strings and coordinates. No training corpus is required. Loading is synchronous and proportional to vocabulary size times width.

## Parameters

`filepath`
: Existing model-artifact path. The view is consumed during the synchronous read.

## Errors

Unlike status-bearing inference calls, artifact failure throws the library's public runtime diagnostic: an empty path, unreadable file, unsupported or incomplete header, checksum mismatch, malformed entry, inconsistent width, or incorrect count is a hard load failure. No partially loaded model is returned.

## Practical Examples

### Restore exact vectors

```cpp
const auto restored = embed::Embedder::load(path);
std::cout << std::boolalpha << "ready=" << restored.ready()
          << " exact=" << (restored.vector_of("cat").value() == cat) << '\n';
```

Output

```text
ready=true exact=true
```

### Restore preprocessing behavior

```cpp
const bool uppercase_rejected = !restored.embed(prep.infer("CAT DOG")).ok();
std::cout << std::boolalpha << uppercase_rejected << '\n';
```

Output

```text
true
```

The saved model used surface terms with `lowercase=false`; restoration does not silently revert that policy.

### Replace a configured object

```cpp
embed::Embedder model({.width = 4});
model = embed::Embedder::load(path);
std::cout << model.width() << ' ' << model.vocabulary().size() << '\n';
```

Output

```text
12 8
```

## See Also

- [`save()`](save.md) writes the matching artifact.
- [`ready()`](ready.md) distinguishes configured and restored state.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn.html">
    <span class="label">Previous</span>
    <span class="title">learn</span>
  </a>
  <a class="next-link" href="nearest.html">
    <span class="label">Next</span>
    <span class="title">nearest</span>
  </a>
</div>
<!-- apiref:nav:end -->
