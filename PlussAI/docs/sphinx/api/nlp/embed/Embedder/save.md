<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: dbab95461351 -->
# `pluss::ai::nlp::embed::Embedder::save`

[Back to `Embedder`](../Embedder.md)

## Signature

```cpp
void pluss::ai::nlp::embed::Embedder::save(std::string_view filepath) const;
```

## What It Does

`save()` writes the ready model's width, preprocessing policy, ordered vocabulary, full-precision vectors, schema, and a checksum. It is a model artifact, not a training checkpoint: optimizer moments, corpus, epoch position, and random-engine state are intentionally absent because loading serves inference rather than training continuation.

The call writes synchronously to the exact path and does not create parent directories. Saving does not mutate model state. Cost and file size scale with vocabulary size times width.

## Parameters

`filepath`
: Destination file path. Parent directories must already exist.

## Errors

An empty path, unready model, failure to open the destination, or write failure throws a public runtime diagnostic. Callers that need atomic replacement should save to a temporary path and rename it after success.

## Practical Examples

### Save and restore

```cpp
model.save("./animals.embed");
const auto restored = embed::Embedder::load("./animals.embed");
std::cout << std::boolalpha
          << (restored.vector_of("cat").value() == model.vector_of("cat").value()) << '\n';
```

Output

```text
true
```

### Temporary artifact

```cpp
const auto path = std::filesystem::temp_directory_path() / "animals.embed";
model.save(path.string());
std::cout << std::filesystem::exists(path) << '\n';
std::filesystem::remove(path);
```

Output

```text
true
```

### Reject untrained state

```cpp
embed::Embedder configured{};
bool rejected = false;
try {
  configured.save("./unused.embed");
} catch (const std::exception&) { rejected = true; }
std::cout << std::boolalpha << rejected << '\n';
```

Output

```text
true
```

The model is rejected before a valid artifact is produced.

## See Also

- [`load()`](load.md) verifies and restores this format.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ready.html">
    <span class="label">Previous</span>
    <span class="title">ready</span>
  </a>
  <a class="next-link" href="similarity.html">
    <span class="label">Next</span>
    <span class="title">similarity</span>
  </a>
</div>
<!-- apiref:nav:end -->
