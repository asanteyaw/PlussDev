<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 740bdc34779d -->

# Archive::label

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::label() const -> ArchiveLabelAccessor
```

## What It Does

The entry point into the archive's **depth labels** — an optional per-catalog label axis,
set via the constructor's `depth_labels` parameter or via `label().set(...)`. `label()`
binds the current archive and hands back an `ArchiveLabelAccessor`:

### `ArchiveLabelAccessor` members

| Member | Kind or type | Signature or default |
|---|---|---|
| `has` | Method | `bool has() const` |
| `values` | Method | `Stream values() const` |
| `set` | Method | `Archive set(const Stream& labels) const` |
| `reset` | Method | `Archive reset() const` |
| `sort` | Method | `Archive sort(bool ascending = true, bool nulls_last = true) const` |
| `align` | Method | `Archive align(const Stream& labels) const` |

```{important} values() throws when no depth labels are set — unlike Stream::lbl().values()
Unlike [`Stream::lbl()`](../stream/lbl.md)`.values()` (which synthesizes a positional
`0..n-1` range when unlabeled), `Archive::label().values()` throws `"Archive::label().values:
depth labels are not set"` if no depth labels have been set. Always check
[`has()`](label.md) first, or catch the throw, before calling `values()`.
```

```{important} align() throws on any unmatched target label — it does not fill with NULL
`align(target_labels)` requires every label in `target_labels` to exist among the
archive's own depth labels; a target label with no match throws `"Archive::label().align:
label not found '<label>'"`. This is stricter than `Slate::align`/`Stream::lbl().align()`,
which fill unmatched positions with `NULL` (or a supplied `fill` value) instead of
throwing.
```

## Parameters

`None`

## Returns

`ArchiveLabelAccessor`
    A binder for the current archive — call one of its methods to inspect, set, reset,
    sort, or align its depth labels.

## Examples

Example 1. `has()`/`values()` on an archive with no depth labels set.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})) });
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0})) });
auto archive = pluss::kindle::Archive({c0, c1});

std::cout << "has(): " << archive.label().has() << "\n";
try {
  archive.label().values();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
has(): 0
Archive::label().values: depth labels are not set
```

Example 2. Set depth labels via the accessor and read them back.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})) });
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0})) });
auto archive = pluss::kindle::Archive({c0, c1});

auto labeled = archive.label().set(pluss::kindle::Stream(pluss::make_note({"train", "test"})));
std::cout << "has(): " << labeled.label().has() << "\n";
std::cout << labeled.label().values();
```

### Output

```text
has(): 1
========================================================================================================================
kindle::Stream  kind=Note  dtype=Note  records=2
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | train                                                                                                    
1            | test                                                                                                     
========================================================================================================================
```

Example 3. `sort()` reorders the catalogs along with the labels, not just the label
values in isolation.

```cpp
using namespace pluss::kindle::literals;

auto c_first = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({100.0})) });
auto c_second = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({200.0})) });
auto archive = pluss::kindle::Archive({c_first, c_second},
    pluss::kindle::Stream(pluss::make_note({"b", "a"})));

auto sorted = archive.label().sort();
std::cout << "sorted labels:\n" << sorted.label().values();
std::cout << "catalog(0):\n" << sorted.catalog(0);
```

### Output

```text
sorted labels:
========================================================================================================================
kindle::Stream  kind=Note  dtype=Note  records=2
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | a                                                                                                        
1            | b                                                                                                        
========================================================================================================================
catalog(0):
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 1       | [200]                                            
========================================================================================================================
```

Sorting by label (`"a"` before `"b"`) moved the catalog originally at depth-index 1 (`id=200`,
labeled `"a"`) to depth-index 0 — `sort()` reorders the whole archive, not just the label
stream.

Example 4. `align()` reorders/filters to `target_labels`; every target label must exist,
or the whole call throws.

```cpp
using namespace pluss::kindle::literals;

auto c_first = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({100.0})) });
auto c_second = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({200.0})) });
auto archive = pluss::kindle::Archive({c_first, c_second},
    pluss::kindle::Stream(pluss::make_note({"b", "a"})));

auto aligned = archive.label().align(pluss::kindle::Stream(pluss::make_note({"a", "b"})));
std::cout << "aligned catalog(0):\n" << aligned.catalog(0);

try {
  archive.label().align(pluss::kindle::Stream(pluss::make_note({"a", "c"})));
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
aligned catalog(0):
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 1       | [200]                                            
========================================================================================================================
Archive::label().align: label not found 'c'
```

## Notes

- `label()` alone does no work — it only binds the archive; call one of its methods to
  actually inspect, set, reset, sort, or align.
- `values()` throws rather than synthesizing a fallback when unlabeled — see the first
  important callout above. Check [`has()`](label.md) first.
- `align()` throws on any unmatched target label rather than filling with `NULL` — see the
  second important callout above. This is stricter than `Slate`/`Stream`'s own `align()`.
- `sort()`/`align()` both reorder the underlying catalogs together with the label stream —
  see Example 3.

## See Also

- [`Stream::lbl`](../stream/lbl.md): the analogous per-row label accessor on `Stream`, with
  different (more forgiving) `values()`/`align()` behavior.
- [`Archive::catalog`](catalog.md): access a catalog by depth-index after sorting/aligning.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kindelize.html">
    <span class="label">Previous</span>
    <span class="title">kindelize</span>
  </a>
  <a class="next-link" href="loc.html">
    <span class="label">Next</span>
    <span class="title">loc</span>
  </a>
</div>
