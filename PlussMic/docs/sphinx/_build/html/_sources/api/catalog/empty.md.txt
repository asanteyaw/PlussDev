<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 879f78c5f493 -->

# Catalog::empty

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::empty() const -> bool
```

## What It Does

Reports whether the catalog has **zero streams** — `nkeys() == 0`. This is a question about
the catalog's *width* (how many named streams it holds), not about whether any individual
stream has zero rows.

```{important} A catalog with streams that each have zero rows is not empty()
`empty()` only looks at `nkeys()`. A catalog holding one or more streams, each of which
happens to have zero elements, still reports `empty() == false` — it has streams, they're
just short. See Example 2.
```

## Parameters

`None`

## Returns

`bool`
    `true` if the catalog has no named streams; `false` otherwise. Requires a defined
    catalog; throws on an undefined one.

## Examples

Example 1. A catalog with streams is not empty.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

std::cout << std::boolalpha << ct.empty();
```

### Output

```text
false
```

Example 2. A stream with zero rows still counts as a stream — the catalog is not
`empty()`, even though the stream itself is short.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series(std::vector<double>{})),
});

std::cout << std::boolalpha << ct.empty() << "\n";
std::cout << ct.nkeys();
```

### Output

```text
false
1
```

Example 3. An explicitly empty stream list (zero streams) is `empty()`.

```cpp
pluss::kindle::Catalog ct(std::vector<std::pair<std::string, pluss::kindle::Stream>>{});
std::cout << std::boolalpha << ct.empty();
```

### Output

```text
true
```

## Notes

- Equivalent to `nkeys() == 0` — a question about stream count, not row count. See the
  important callout above.
- Throws `"Catalog: undefined handle"` on an undefined catalog — see
  [`defined()`](defined.md).

## See Also

- [`Catalog::nkeys`](nkeys.md): the value `empty()` compares against zero.
- [`Catalog::defined`](defined.md): a different, prerequisite question — whether the
  catalog has a backing handle at all.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropnull.html">
    <span class="label">Previous</span>
    <span class="title">dropnull</span>
  </a>
  <a class="next-link" href="end.html">
    <span class="label">Next</span>
    <span class="title">end</span>
  </a>
</div>
