<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: 97fb0c16693b -->
# `pluss::ai::nlp::embed::Embedder::nearest`

[Back to `Embedder`](../Embedder.md)

## Signature

```cpp
pluss::ai::nlp::StatusOr<std::vector<std::pair<std::string, double>>>
pluss::ai::nlp::embed::Embedder::nearest(
    std::string_view token, std::int64_t k = 10) const;
```

## What It Does

`nearest()` computes cosine similarity from the query term's learned vector to every other vocabulary row, excludes the query itself, and returns the highest-scoring prefix in descending score order. It is an exact exhaustive search, not an approximate index.

`k=0` succeeds with an empty vector. A request larger than the number of other terms returns every available neighbor. Negative `k`, an unknown exact term, or an unready model returns a failed status. Equal scores have no promised secondary ordering.

The operation costs one width-sized cosine calculation per vocabulary entry and a partial sort of the requested prefix. Repeated large-vocabulary queries should be served by a purpose-built external index.

## Parameters

`token`
: The exact vocabulary key whose row is the query.

`k`
: Maximum number of other terms to return; defaults to `10` and must be non-negative.

## Practical Examples

### Top three

```cpp
for (const auto& [term, score] : model.nearest("cat", 3).value())
  std::cout << term << ':' << std::fixed << std::setprecision(3) << score << ' ';
std::cout << '\n';
```

Output

```text
animal:0.439 dog:0.409 pet:0.242 
```

### Empty and oversized requests

```cpp
std::cout << model.nearest("cat", 0).value().size() << '\n';
std::cout << model.nearest("cat", 1000).value().size() << '\n';
```

Output

```text
0
7
```

### Invalid count

```cpp
std::cout << model.nearest("cat", -1).status().message << '\n';
```

Output

```text
nlp::embed::Embedder::nearest: k must be non-negative.
```

### Unknown query

```cpp
std::cout << model.nearest("moon", 3).status().message << '\n';
```

Output

```text
nlp::embed::Embedder::nearest: 'moon' is out of vocabulary.
```

## See Also

- [`similarity()`](similarity.md) compares two supplied vectors.
- [`contains()`](contains.md) performs a cheap membership check.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load.html">
    <span class="label">Previous</span>
    <span class="title">load</span>
  </a>
  <a class="next-link" href="ready.html">
    <span class="label">Next</span>
    <span class="title">ready</span>
  </a>
</div>
<!-- apiref:nav:end -->
