<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: f8d28f64c7a3 -->
# `pluss::ai::nlp::embed::Embedder::vocabulary`

[Back to `Embedder`](../Embedder.md)

## Signature

```cpp
const std::vector<std::string>&
pluss::ai::nlp::embed::Embedder::vocabulary() const noexcept;
```

## What It Does

`vocabulary()` borrows the ordered term list whose indices correspond to learned vector rows. Terms are frequency ordered with deterministic lexical tie handling from training. The reference is empty before training and remains valid until the model object is destroyed or receives replacement state through assignment or `learn()`.

The call is O(1) and copies nothing. Do not retain the reference across an operation that can replace model state. Use `contains()` when only membership is needed.

## Practical Examples

### Inspect size and order

```cpp
std::cout << "count=" << model.vocabulary().size() << " first="
          << model.vocabulary().front() << '\n';
```

Output

```text
count=8 first=animal
```

### Untrained vocabulary

```cpp
embed::Embedder configured{};
std::cout << configured.vocabulary().size() << '\n';
```

Output

```text
0
```

### Traverse rows consistently

```cpp
for (const std::string& term : model.vocabulary()) {
  const auto row = model.vector_of(term);
  if (!row.ok() || row.value().size() != static_cast<std::size_t>(model.width())) return 1;
}
std::cout << "all rows match width\n";
```

Output

```text
all rows match width
```

## See Also

- [`contains()`](contains.md) performs exact membership lookup.
- [`nearest()`](nearest.md) returns scored terms rather than the whole list.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="vector_of.html">
    <span class="label">Previous</span>
    <span class="title">vector_of</span>
  </a>
  <a class="next-link" href="width.html">
    <span class="label">Next</span>
    <span class="title">width</span>
  </a>
</div>
<!-- apiref:nav:end -->
