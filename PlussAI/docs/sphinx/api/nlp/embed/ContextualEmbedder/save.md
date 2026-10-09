<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: a58ca27c399e -->
# `pluss::ai::nlp::embed::ContextualEmbedder::save`

[Back to `ContextualEmbedder`](../ContextualEmbedder.md)

## Signature

```cpp
void pluss::ai::nlp::embed::ContextualEmbedder::save(std::string_view filepath) const;
```

## What It Does

`save()` writes all state needed for contextual inference: indexer and preprocessing state, embeddings, architecture, Transformer parameters, output projection, schema, and checksum. It does not save optimizer state or resume an interrupted `learn()`.

Writing is synchronous and does not create parent directories. File size is dominated by model parameters. An empty path, unready model, open failure, or write failure throws a public runtime diagnostic.

## Parameters

`filepath`
: Destination artifact path. Parent directories must already exist.

## Practical Examples

### Persist and verify

```cpp
model.save("./language.ctx");
const auto restored = embed::ContextualEmbedder::load("./language.ctx");
std::cout << std::boolalpha
          << (restored.encode(sentence).value() == model.encode(sentence).value()) << '\n';
```

Output

```text
true
```

### Temporary path

```cpp
const auto path = std::filesystem::temp_directory_path() / "language.ctx";
model.save(path.string());
std::cout << (std::filesystem::file_size(path) > 0) << '\n';
std::filesystem::remove(path);
```

Output

```text
true
```

### Unready model

```cpp
bool rejected = false;
try {
  embed::ContextualEmbedder(options).save("./unused.ctx");
} catch (const std::exception&) { rejected = true; }
std::cout << std::boolalpha << rejected << '\n';
```

Output

```text
true
```

The call fails before opening a valid model artifact.

## See Also

- [`load()`](load.md) validates and restores the artifact.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ready.html">
    <span class="label">Previous</span>
    <span class="title">ready</span>
  </a>
  <a class="next-link" href="width.html">
    <span class="label">Next</span>
    <span class="title">width</span>
  </a>
</div>
<!-- apiref:nav:end -->
