<!-- pluss-hand-authored -->

# Registry::sample

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::sample(std::int64_t n, std::optional<std::uint64_t> random_state = std::nullopt) const
```

## What It Does

Returns a new registry with `n` randomly selected rows, drawn without replacement
and applied identically across every field via a single shuffled index list
(internally, `take()` with a Fisher–Yates-shuffled `[0, size)` index range,
truncated to `n`). Unlike every other row-selection method on `Registry`
(`head()`, `tail()`, `slice()`, `cloc()`, `take()`), which apply independently
per field, **`sample()` requires every field to share the same length**.

```{important} sample() requires an aligned (non-ragged) registry
Because `sample()` builds one shared shuffled index list from the *first* field's
length, then applies it to every field via `take()`, a registry whose fields don't
all share the same length throws
`"Registry::sample: requires aligned slot lengths"` — this is a stricter
requirement than every other row-selection method on `Registry`, none of which
require field-length agreement. See Example 2.
```

## Parameters

**n** (`std::int64_t`)
    The number of rows to sample. Clamped to `[0, size]` — requesting more than
    the registry's row count returns all rows (still shuffled), not a throw.

**random_state** (`std::optional<std::uint64_t>`, optional)
    A seed for the shuffle. Passing the same seed on the same registry reproduces
    the same sample; omitting it seeds from `std::random_device`, giving a
    different result on each call.

## Returns

`Registry`
    A new registry with `n` rows, one per field, selected via the same shuffled
    index list across all fields.

## Examples

Example 1. The same seed reproduces the same sample.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi"), pluss::make_note("Ho")})},
});

auto first = rg.sample(2, 42);
auto second = rg.sample(2, 42);

bool same_ids = true;
for (std::int64_t i = 0; i < first.select("id").size(); ++i) {
  if (std::get<pluss::Series>(first.select("id").at(i)).to_vec<std::int64_t>()[0] !=
      std::get<pluss::Series>(second.select("id").at(i)).to_vec<std::int64_t>()[0]) {
    same_ids = false;
  }
}
std::cout << "size: " << first.size() << ", reproducible: " << std::boolalpha << same_ids;
```

### Output

```text
size: 2, reproducible: true
```

Example 2. A ragged registry (fields of different lengths) throws.

```cpp
auto ragged = pluss::bundle::Registry({
    {"a", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"b", pluss::bundle::Slot({pluss::make_series(10), pluss::make_series(20)})},
});

try {
  auto out = ragged.sample(1, 42);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Registry::sample: requires aligned slot lengths
```

## Notes

- Requires field-length alignment — see the important callout above. Every other
  row-selection method on `Registry` tolerates a ragged registry; this one does
  not.
- `n` is clamped to `[0, size]`; it never throws for an out-of-range `n` the way it
  does for misaligned fields.
- Without `random_state`, results are non-deterministic (seeded from
  `std::random_device`) — pass a seed whenever reproducibility matters (tests,
  debugging).
- An empty registry (`empty() == true`) returns itself unchanged, regardless of
  `n`.

## See Also

- [`Registry::take`](take.md): the underlying selection mechanism `sample()` builds
  its shuffled index list for.
- [`Registry::is_stream_compatible`](is_stream_compatible.md): a related but
  distinct per-field uniformity check — value-kind uniformity, not length
  uniformity.
- [`Catalog::sample`](../catalog/sample.md): the equivalent operation on the
  kindle model.

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
