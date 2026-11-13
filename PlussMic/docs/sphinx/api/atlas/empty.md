<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 879f78c5f493 -->

# Atlas::empty

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
bool Atlas::empty() const
```

## What It Does

Reports whether the atlas has zero frames — `depth() == 0` — with one extra
convenience: it's also safe to call on an undefined `Atlas`, unlike `depth()` itself.
A default-constructed `Atlas()` is not defined at all ([`defined()`](defined.md) reports
`false` for it), and `empty()` treats that case as empty too, rather than throwing.

## Parameters

`None`

## Returns

`bool`
    `true` if the atlas is undefined, or defined with zero frames; `false` if it holds
    one or more frames.

## Examples

Example 1. A non-empty atlas.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"sales", Slate(pluss::make_series({100.0, 120.0}))}});
auto q2 = DataFrame({{"sales", Slate(pluss::make_series({110.0, 90.0}))}});
auto atlas = Atlas({q1, q2});

std::cout << atlas.empty();
```

### Output

```text
0
```

Example 2. An atlas constructed from an empty vector of frames is empty, but still
defined.

```cpp
using namespace pluss::table;

auto atlas = Atlas(std::vector<DataFrame>{});
std::cout << "defined: " << atlas.defined() << "\n";
std::cout << "empty: " << atlas.empty() << "\n";
std::cout << "depth: " << atlas.depth();
```

### Output

```text
defined: 1
empty: 1
depth: 0
```

Example 3. A default-constructed atlas is empty too, even though it isn't defined —
`empty()` doesn't throw here the way `depth()` would.

```cpp
using namespace pluss::table;

Atlas undefined_atlas;
std::cout << "defined: " << undefined_atlas.defined() << "\n";
std::cout << "empty: " << undefined_atlas.empty();
```

### Output

```text
defined: 0
empty: 1
```

## Notes

- `empty()` is one of the few `Atlas` methods safe to call on an undefined `Atlas` — it
  reports `true` instead of throwing.
- Distinct from `defined()`: an atlas can be defined and still empty (Example 2), and an
  undefined atlas is treated as empty rather than raising an error (Example 3).

## See Also

- [`Atlas::depth`](depth.md): the exact frame count this method checks against zero;
  throws on an undefined `Atlas`, unlike `empty()`.
- [`Atlas::defined`](defined.md): whether the atlas has a payload at all, independent of
  frame count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dloc.html">
    <span class="label">Previous</span>
    <span class="title">dloc</span>
  </a>
  <a class="next-link" href="features.html">
    <span class="label">Next</span>
    <span class="title">features</span>
  </a>
</div>
