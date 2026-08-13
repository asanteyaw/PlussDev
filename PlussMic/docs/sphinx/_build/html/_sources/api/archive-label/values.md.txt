<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 62349bfebda1 -->

# ArchiveLabelAccessor::values

[Back to ArchiveLabelAccessor API Reference](../../archive_label_accessor_api_reference)

## Signature

```cpp
Stream ArchiveLabelAccessor::values() const
```

## What It Does

Returns the archive's current depth labels as a `Stream`, one entry per catalog in the
stack.

```{important} Throws when unlabeled — no synthesized fallback
Unlike [`StreamLabelAccessor::values()`](../stream-lbl/values.md), which always
succeeds and synthesizes a positional `0..n-1` range when the stream is unlabeled,
`ArchiveLabelAccessor::values()` throws `"Archive::label().values: depth labels are not
set"` outright if [`has()`](has.md) is `false`. Check `has()` first, or be prepared to
catch the throw.
```

## Parameters

`None`

## Returns

`Stream`
    The archive's depth labels, with length equal to `depth()`.

## Examples

Example 1. `values()` on a labeled archive.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({10.0})) });
auto c1 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({20.0})) });
auto c2 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({30.0})) });

auto labeled = pluss::kindle::Archive({c0, c1, c2})
                   .label().set(pluss::kindle::Stream(pluss::make_note({"q1", "q2", "q3"})));

std::cout << labeled.label().values();
```

### Output

```text
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

Example 2. Calling `values()` on an unlabeled archive throws — see the important
callout above.

```cpp
auto c0 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({10.0})) });
auto archive = pluss::kindle::Archive({c0});

try {
  auto v = archive.label().values();
} catch (const std::exception& e) {
  std::cout << "threw: " << e.what() << "\n";
}
```

### Output

```text
threw: Archive::label().values: depth labels are not set
```

## Notes

- See the important callout above.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise (a
  separate check from the "labels are not set" throw above).
- The returned `Stream`'s own kind matches whatever kind was passed to
  [`set()`](set.md) — `Note`, `Series`, `Timeline`, or `TimeIndex` are all valid depth
  label kinds.

## See Also

- [`ArchiveLabelAccessor::has`](has.md): check before calling, to avoid the throw.
- [`ArchiveLabelAccessor::set`](set.md): attach depth labels.
- [`Archive::label`](../archive/label.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html">
    <span class="label">Previous</span>
    <span class="title">sort</span>
  </a>
</div>
