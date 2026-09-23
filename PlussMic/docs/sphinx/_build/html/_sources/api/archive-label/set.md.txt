<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: eaa991f9087e -->

# ArchiveLabelAccessor::set

[Back to ArchiveLabelAccessor API Reference](../../archive_label_accessor_api_reference)

## Signature

```cpp
Archive ArchiveLabelAccessor::set(const Stream& labels) const
```

## What It Does

Attaches `labels` as the archive's depth labels — one label per catalog in the stack —
returning a new archive. This is the *depth*-axis equivalent of
[`StreamLabelAccessor::set`](../stream-lbl/set.md), labeling which catalog is which
within the stack, not any row axis inside an individual catalog.

```{important} labels length must equal depth() exactly
`labels.size()` must equal the archive's current `depth()` — a mismatch throws
`"Archive: depth labels length must match catalog depth"`. There is no partial-labeling
or auto-padding; every catalog in the stack gets exactly one label, in order.
```

## Parameters

**labels** (`const Stream&`)
    The new depth labels, one entry per catalog. Any payload kind is accepted — `Note`
    (the common case, e.g. quarter names) works as well as `Series`, `Timeline`, or
    `TimeIndex`.

## Returns

`Archive`
    A new archive with `labels` attached as its depth labels; the catalog stack itself
    (contents and order) is unchanged.

## Examples

Example 1. Attach `Note`-kind quarter labels to a 3-deep archive.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({10.0})) });
auto c1 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({20.0})) });
auto c2 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({30.0})) });
auto archive = pluss::kindle::Archive({c0, c1, c2});

auto labeled = archive.label().set(pluss::kindle::Stream(pluss::make_note({"q1", "q2", "q3"})));
std::cout << "has(): " << std::boolalpha << labeled.label().has() << "\n";
std::cout << labeled.label().values();
```

### Output

```text
has(): true
========================================================================================================================
kindle::Stream  kind=Note  dtype=Note  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | q1                                                                                                       
1            | q2                                                                                                       
2            | q3                                                                                                       
========================================================================================================================
```

Example 2. A length mismatch throws — `labels` must have exactly one entry per catalog.

```cpp
auto c0 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({10.0})) });
auto c1 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({20.0})) });
auto c2 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({30.0})) });
auto archive = pluss::kindle::Archive({c0, c1, c2});

try {
  auto labeled = archive.label().set(pluss::kindle::Stream(pluss::make_note({"q1", "q2"})));
} catch (const std::exception& e) {
  std::cout << "threw: " << e.what() << "\n";
}
```

### Output

```text
threw: Archive: depth labels length must match catalog depth
```

## Notes

- See the important callout above.
- After `set()`, [`has()`](has.md) reports `true`.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.
- Calling `set()` again on an already-labeled archive replaces the existing labels
  outright — it does not merge or append.

## See Also

- [`ArchiveLabelAccessor::reset`](reset.md): the inverse operation, clearing depth
  labels back to unset.
- [`ArchiveLabelAccessor::has`](has.md): confirms a label was genuinely set.
- [`Archive::label`](../archive/label.md): the entry point this accessor is reached
  from — its own `Archive(catalogs, depth_labels)` constructor accepts an optional
  `depth_labels` argument as the equivalent way to set depth labels up front.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reset.html">
    <span class="label">Previous</span>
    <span class="title">reset</span>
  </a>
  <a class="next-link" href="sort.html">
    <span class="label">Next</span>
    <span class="title">sort</span>
  </a>
</div>
