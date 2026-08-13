<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4d57e20373a1 -->

# ArchiveLabelAccessor::align

[Back to ArchiveLabelAccessor API Reference](../../archive_label_accessor_api_reference)

## Signature

```cpp
Archive ArchiveLabelAccessor::align(const Stream& labels) const
```

## What It Does

Reorders (and narrows or expands) the archive's depth stack so that catalog `i` of the
result is whichever catalog currently carries depth label `labels[i]`. This is a
label-lookup reorder, not a positional one — every entry in `labels` is matched by
content against the archive's *current* depth labels.

```{important} Requires the archive to already have depth labels
Unlike [`StreamLabelAccessor::align`](../stream-lbl/align.md) — which has an
`AlignBy::Pos` default that works on an unlabeled stream — `ArchiveLabelAccessor::align`
has no positional mode at all. It always matches by label content, and it throws
`"Archive::label().align: current archive has no depth labels"` immediately if
[`has()`](has.md) is `false`. Call [`set(labels)`](set.md) first if the archive doesn't
already carry depth labels.
```

```{important} No fill option — every requested label must exist, and be unique
Every entry in `labels` must match exactly one of the archive's current depth labels,
or `align()` throws `"Archive::label().align: label not found '...'"`. There is no
`options.fill` escape hatch here, unlike [`Stream::lbl().align`](../stream-lbl/align.md).
The archive's *current* depth labels must also be unique among themselves — a duplicate
current label throws `"Archive::label().align: duplicate current depth label '...'"`
before any lookup happens.
```

## Parameters

**labels** (`const Stream&`)
    The depth-label values to align onto, in the desired result order. Its length
    becomes the result's new `depth()` — it can be shorter (narrowing), longer than is
    possible only up to the number of distinct current labels, reordered, or repeat a
    label to duplicate that catalog in the result.

## Returns

`Archive`
    A new archive whose depth stack is reordered/narrowed to match `labels`, with
    `labels` itself becoming the result's new depth labels.

## Examples

Example 1. Reorder and narrow a 3-catalog archive down to 2, in a new order.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({10.0})) });
auto c1 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({20.0})) });
auto c2 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({30.0})) });

auto labeled = pluss::kindle::Archive({c0, c1, c2})
                   .label().set(pluss::kindle::Stream(pluss::make_note({"a", "b", "c"})));

auto aligned = labeled.label().align(pluss::kindle::Stream(pluss::make_note({"c", "a"})));
std::cout << "aligned depth: " << aligned.depth() << "\n";
std::cout << aligned.catalog(0);
std::cout << aligned.catalog(1);
```

### Output

```text
aligned depth: 2
========================================================================================================================
kindle::Catalog  keys=1
keys: [px]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
px                       | Series       | Series[float64]  | 1       | [30]                                             
========================================================================================================================
========================================================================================================================
kindle::Catalog  keys=1
keys: [px]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
px                       | Series       | Series[float64]  | 1       | [10]                                             
========================================================================================================================
```

The result has depth 2, matching `labels`' length: catalog 0 is the archive's `"c"`
catalog (`px=30`), catalog 1 is its `"a"` catalog (`px=10`) — `"b"` (`px=20`) is simply
dropped since it wasn't requested.

Example 2. An unrecognized target label throws immediately, before any reordering.

```cpp
auto c0 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({10.0})) });
auto c1 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({20.0})) });

auto labeled = pluss::kindle::Archive({c0, c1})
                   .label().set(pluss::kindle::Stream(pluss::make_note({"a", "b"})));

try {
  auto aligned = labeled.label().align(pluss::kindle::Stream(pluss::make_note({"z"})));
} catch (const std::exception& e) {
  std::cout << "threw: " << e.what() << "\n";
}
```

### Output

```text
threw: Archive::label().align: label not found 'z'
```

## Notes

- See both important callouts above — `align()` always matches by label content
  (no positional mode), requires the archive to already be labeled, and offers no fill
  fallback for an unmatched target label.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.
- `labels` becomes the result's own depth labels — `has()` is `true` on the result.

## See Also

- [`ArchiveLabelAccessor::set`](set.md): attach depth labels first if `has()` is
  currently `false`.
- [`ArchiveLabelAccessor::sort`](sort.md): reorder by label *value* rather than by an
  explicit target list.
- [`Stream::lbl().align`](../stream-lbl/align.md): the row-label equivalent on a single
  `Stream`, which additionally supports positional alignment and a fill value.
- [`Archive::label`](../archive/label.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
