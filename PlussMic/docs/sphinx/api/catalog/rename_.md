<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 061245b4b47e -->

# Catalog::rename_

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::rename_(std::string_view from, std::string_view to) -> Catalog&
```

## What It Does

The in-place counterpart to [`rename(from, to)`](rename.md) — reassigns `*this` to the
renamed result and returns a reference to it, rather than returning a new `Catalog`.
Internally it's exactly `*this = rename(from, to); return *this;`, so every validation rule
and throw condition from `rename()` applies unchanged.

## Parameters

**from** (`std::string_view`)
    The existing stream name.

**to** (`std::string_view`)
    The new name for that stream.

## Returns

`Catalog&`
    A reference to `*this`, after the rename.

## Examples

Example 1. Rename a stream in place; the return value is a reference to the same object.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

ct.rename_("id", "identifier");
std::cout << ct;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [identifier, metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
identifier               | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
metric                   | Series       | Series[float64]  | 3       | [10, 20, 30]                                     
========================================================================================================================
```

## Notes

- Implemented as `*this = rename(from, to)` — every throw condition documented on
  [`rename()`](rename.md) (unknown source key, empty target name, colliding target name)
  applies here unchanged.
- Only the single-pair form has a mutating counterpart — there is no `rename_` overload
  taking a mapping.

## See Also

- [`Catalog::rename`](rename.md): the non-mutating form, with the full set of overloads
  and throw conditions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rename.html">
    <span class="label">Previous</span>
    <span class="title">rename</span>
  </a>
  <a class="next-link" href="sample.html">
    <span class="label">Next</span>
    <span class="title">sample</span>
  </a>
</div>
