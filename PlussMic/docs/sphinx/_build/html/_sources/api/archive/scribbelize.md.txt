<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5fa93610e08b -->

# Archive::scribbelize

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
pluss::scribble::Axel Archive::scribbelize() const
```

## What It Does

Converts the archive into a `pluss::scribble::Axel` — the reverse direction of
[`Axel::kindelize`](../axel/kindelize.md). An `Archive` is a depth-stack of
`Catalog`s plus an optional per-depth-level labels stream; an `Axel` is the scribble
model's equivalent depth-stack of `Registry`s plus an optional per-level labels
`Slot`. `scribbelize()` converts each level via
[`Catalog::scribbelize`](../catalog/scribbelize.md) (in depth order, level `0` first) and,
if the archive carries depth labels, converts those too via
[`Stream::scribbelize`](../stream/scribbelize.md) — otherwise the resulting `Axel`'s
labels are left empty.

## Parameters

`None`

## Returns

`pluss::scribble::Axel`
    A new axel with one `Registry` per archive depth level, in depth order, and depth
    labels present if and only if the source archive's own
    [`label().has()`](label.md) was `true`.

## Examples

Example 1. A two-level archive with depth labels — `scribbelize()` preserves both the
depth count and the labels.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));
auto axel = archive.scribbelize();
std::cout << axel.depth() << "\n";
std::cout << axel.labels().has_value() << "\n";
std::cout << axel.labels()->size() << "\n";
```

### Output

```text
2
1
2
```

Example 2 (edge case). An archive constructed **without** depth labels produces an
`Axel` whose `labels()` is empty, not a default-constructed placeholder `Slot`.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive_no_labels = pluss::kindle::Archive({c0, c1});
auto axel_no_labels = archive_no_labels.scribbelize();
std::cout << axel_no_labels.labels().has_value() << "\n";
```

### Output

```text
0
```

Example 3 (edge case). An undefined archive throws before any level is touched.

```cpp
try {
  pluss::kindle::Archive undefined_archive;
  undefined_archive.scribbelize();
} catch (const pluss::Error& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Archive::scribbelize: archive undefined
```

## Notes

- Requires `defined()` — an undefined archive throws
  `"Archive::scribbelize: archive undefined"` immediately, before any level is visited.
- Depth-level order is always preserved: level `i` of the source archive
  ([`catalog(i)`](catalog.md)) becomes registry `i` of the resulting axel.
- Depth labels are converted through the exact same
  [`Stream::scribbelize`](../stream/scribbelize.md) path as any other stream — including
  its null-preservation behavior (masked-out label positions become
  `std::monostate{}` in the resulting `Slot`), not a special label-only code path.
- An empty archive (zero depth levels, but still `defined()`) produces an axel with
  zero registries.

## See Also

- [`Axel::kindelize`](../axel/kindelize.md): the reverse bridge, from the scribble model
  back into Kindle.
- [`Catalog::scribbelize`](../catalog/scribbelize.md): what this method calls once per
  depth level.
- [`Stream::scribbelize`](../stream/scribbelize.md): what this method calls on the depth
  labels, when present.
- [`Archive::label`](label.md): what determines whether the resulting axel carries
  labels.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bfill.html">
    <span class="label">Previous</span>
    <span class="title">bfill</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
