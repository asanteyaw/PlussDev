<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: 2c6c5f00ac69 -->
# `pluss::ai::nlp::embed::Embedder::similarity`

[Back to `Embedder`](../Embedder.md)

## Signature

```cpp
static double pluss::ai::nlp::embed::Embedder::similarity(
    const std::vector<double>& lhs,
    const std::vector<double>& rhs) noexcept;
```

## What It Does

`similarity()` computes cosine similarity: the dot product divided by both Euclidean norms. It compares direction rather than magnitude, so positive rescaling does not change a nonzero vector's result. Equal directions approach `1`, orthogonal directions produce `0`, and opposite directions approach `-1`.

The helper is independent of model state and accepts any host vectors. Because it is `noexcept` and returns a plain `double`, mismatched lengths, empty inputs, and either zero-norm input return the neutral sentinel `0.0`. Validate dimensions separately when those cases must be distinguished from genuine orthogonality.

## Parameters

`lhs`, `rhs`
: Host vectors to compare. A valid cosine comparison requires equal, nonzero lengths and nonzero norms.

## Practical Examples

### Geometric anchors

```cpp
std::cout << std::fixed << std::setprecision(3)
          << "same=" << embed::Embedder::similarity({1, 0}, {1, 0})
          << " orthogonal=" << embed::Embedder::similarity({1, 0}, {0, 1})
          << " opposite=" << embed::Embedder::similarity({1, 0}, {-1, 0}) << '\n';
```

Output

```text
same=1.000 orthogonal=0.000 opposite=-1.000
```

### Scale invariance

```cpp
std::cout << embed::Embedder::similarity({1, 2}, {3, 6}) << '\n';
```

Output

```text
1.000
```

### Degenerate inputs

```cpp
std::cout << "empty=" << embed::Embedder::similarity({}, {})
          << " mismatch=" << embed::Embedder::similarity({1}, {1, 2})
          << " zero=" << embed::Embedder::similarity({0, 0}, {1, 0}) << '\n';
```

Output

```text
empty=0.000 mismatch=0.000 zero=0.000
```

### Learned terms

```cpp
const auto cat = model.vector_of("cat").value();
const auto dog = model.vector_of("dog").value();
std::cout << std::boolalpha
          << std::isfinite(embed::Embedder::similarity(cat, dog)) << '\n';
```

Output

```text
true
```

This compares two independent vector copies; neither the vectors nor model state are changed.

## See Also

- [`nearest()`](nearest.md) applies this measure across the model vocabulary.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save.html">
    <span class="label">Previous</span>
    <span class="title">save</span>
  </a>
  <a class="next-link" href="vector_of.html">
    <span class="label">Next</span>
    <span class="title">vector_of</span>
  </a>
</div>
<!-- apiref:nav:end -->
