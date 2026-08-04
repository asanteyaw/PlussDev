<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c68e570c183e -->

# Catalog::sample

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::sample(std::int64_t n, std::optional<std::uint64_t> random_state = std::nullopt) const -> Catalog
```

## What It Does

Selects `n` rows at random, without replacement, from every stream in the catalog —
shuffling row positions with a Mersenne Twister engine, then keeping the first `n`
positions via [`take()`](take.md). Unlike the purely positional selectors
(`head`/`tail`/`slice`/`take`), `sample()` requires every stream to share the *same*
length; it throws on a ragged catalog rather than sampling each stream independently.

```{important} Omitting random_state makes the result non-deterministic
Without a `random_state`, `sample()` seeds its shuffle from `std::random_device`, so two
calls — even on the same catalog — can return different rows. Pass an explicit
`random_state` whenever you need reproducible output (tests, documentation, debugging). See
Example 2, where the same seed reliably reproduces the same sample.
```

## Parameters

**n** (`std::int64_t`)
    Number of rows to sample. Clamped to `[0, nrows]` — requesting more rows than the
    catalog has simply returns all of them (still shuffled), rather than throwing.

**random_state** (`std::optional<std::uint64_t>`, default `std::nullopt`)
    An explicit seed for reproducible sampling. Omit it for a non-deterministic sample
    seeded from the system's random device.

## Returns

`Catalog`
    A new catalog with every stream restricted to the same `n` randomly chosen row
    positions.

## Examples

Example 1. Sample 2 of 5 rows with an explicit seed.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi", "Ho", "Cape Coast"})),
});

auto out = ct.sample(2, 42);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, city]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 2       | [1, 4]                                           
city                     | Note         | Note             | 2       | [Accra, Ho]                                      
========================================================================================================================
```

Example 2. The same `random_state` reliably reproduces the same sample.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
});

auto first = ct.sample(2, 42);
auto second = ct.sample(2, 42);
std::cout << std::boolalpha << (first.at("id").to_string() == second.at("id").to_string());
```

### Output

```text
true
```

Example 3. Sampling requires every stream to share the same length — a ragged catalog
throws.

```cpp
auto ragged = pluss::kindle::Catalog({
    "a"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "b"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
});

try {
  auto out = ragged.sample(1);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::sample: requires aligned stream lengths
```

## Notes

- Sampling is without replacement — each row position appears at most once in the result.
- Requires aligned stream lengths, unlike `head`/`tail`/`slice`/`take`; throws on a ragged
  catalog rather than sampling per-stream.
- `n` is clamped to `[0, nrows]`; requesting more rows than exist returns every row
  (shuffled), not an error.
- Non-deterministic unless `random_state` is set — see the important callout above.

## See Also

- [`Catalog::take`](take.md): the positional mechanism `sample()` uses internally, for when
  you already know which positions you want.
- [`Catalog::nrows`](nrows.md): the aligned row-length check `sample()` performs
  before shuffling.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rename_.html">
    <span class="label">Previous</span>
    <span class="title">rename_</span>
  </a>
  <a class="next-link" href="save_as.html">
    <span class="label">Next</span>
    <span class="title">save_as</span>
  </a>
</div>
