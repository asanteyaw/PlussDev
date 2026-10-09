<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b74a75f97174 -->

# ArchiveLabelAccessor::sort

[Back to ArchiveLabelAccessor API Reference](../../archive_label_accessor_api_reference)

## Signature

```cpp
Archive ArchiveLabelAccessor::sort(bool ascending = true, bool nulls_last = true) const
```

## What It Does

Reorders the archive's catalog stack (and its depth labels, if set) by depth-label
value. This is the depth-axis analogue of
[`DataFrame::sort`](../dataframe/sort.md)/`Slate::sort`, but simpler: there's no
builder, no multi-key plan — `sort()` is a single terminal call, ordering strictly by
the depth label itself.

```{important} With no depth labels set, sort() only responds to ascending=false
If [`has()`](has.md) is `false`, there's no label value to sort by. `sort(true)` (the
default) then leaves the catalog order completely unchanged — it is a no-op.
`sort(false)` still does something meaningful in this case: it reverses the current
catalog order, since "descending with no explicit values" is interpreted as reversing
whatever order already exists. See Examples 2 and 3.
```

## Parameters

**ascending** (`bool`, default `true`)
    Sort direction by depth-label value.

**nulls_last** (`bool`, default `true`)
    Where a `NULL`/missing depth label sorts relative to non-null ones — matches the
    same `nulls_last` convention used by [`DataFrame::sort`](../dataframe/sort.md).

## Returns

`Archive`
    A new archive with catalogs (and depth labels, if present) reordered.

## Examples

Example 1. Sorting a labeled archive ascending reorders both the depth labels and
their corresponding catalogs together.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({10.0})) }); // "c"
auto c1 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({20.0})) }); // "a"
auto c2 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({30.0})) }); // "b"

auto labeled = pluss::kindle::Archive({c0, c1, c2})
                   .label().set(pluss::kindle::Stream(pluss::make_note({"c", "a", "b"})));

auto sorted = labeled.label().sort(true);
std::cout << sorted.label().values();
std::cout << sorted.catalog(0);
std::cout << sorted.catalog(1);
std::cout << sorted.catalog(2);
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Note  dtype=Note  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | a                                                                                                        
1            | b                                                                                                        
2            | c                                                                                                        
========================================================================================================================
========================================================================================================================
kindle::Catalog  keys=1
keys: [px]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
px                       | Series       | Series[float64]  | 1       | [20]                                             
========================================================================================================================
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

The depth labels come back `a`, `b`, `c` in order; the catalogs travel with their
labels — `a`'s catalog (`px=20`) is now first, `b`'s (`px=30`) second, `c`'s (`px=10`,
originally first) last.

Example 2. With no depth labels set, `sort(true)` (the default) is a no-op — catalog
order is completely unchanged.

```cpp
auto c0 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({10.0})) });
auto c1 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({20.0})) });
auto c2 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({30.0})) });
auto archive = pluss::kindle::Archive({c0, c1, c2});

auto sorted = archive.label().sort();
std::cout << sorted.catalog(0);
std::cout << sorted.catalog(1);
std::cout << sorted.catalog(2);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [px]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
px                       | Series       | Series[float64]  | 1       | [10]                                             
========================================================================================================================
========================================================================================================================
kindle::Catalog  keys=1
keys: [px]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
px                       | Series       | Series[float64]  | 1       | [20]                                             
========================================================================================================================
========================================================================================================================
kindle::Catalog  keys=1
keys: [px]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
px                       | Series       | Series[float64]  | 1       | [30]                                             
========================================================================================================================
```

Order (`10`, `20`, `30`) is exactly as constructed — see the important callout above.

Example 3. Still with no depth labels set, `sort(false)` reverses the current catalog
order instead of leaving it unchanged.

```cpp
auto c0 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({10.0})) });
auto c1 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({20.0})) });
auto c2 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({30.0})) });
auto archive = pluss::kindle::Archive({c0, c1, c2});

auto sorted = archive.label().sort(false);
std::cout << sorted.catalog(0);
std::cout << sorted.catalog(1);
std::cout << sorted.catalog(2);
```

### Output

```text
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
px                       | Series       | Series[float64]  | 1       | [20]                                             
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

## Notes

- See the important callout above — the unlabeled case is the one behavior most likely
  to surprise: `ascending=true` (the default) silently does nothing without labels.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.
- `depth()` is unchanged by `sort()` — it only reorders, never drops or duplicates
  catalogs.

## See Also

- [`ArchiveLabelAccessor::align`](align.md): reorder to an explicit target label list
  instead of a sort direction.
- [`ArchiveLabelAccessor::has`](has.md): check whether depth labels are set, to predict
  which of the two behaviors above `sort()` will have.
- [`DataFrame::sort`](../dataframe/sort.md): the richer, multi-key row-sort builder on
  `DataFrame` — unrelated axis, but the conceptually nearest sort surface elsewhere in
  the library.
- [`Archive::label`](../archive/label.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set.html">
    <span class="label">Previous</span>
    <span class="title">set</span>
  </a>
  <a class="next-link" href="values.html">
    <span class="label">Next</span>
    <span class="title">values</span>
  </a>
</div>
