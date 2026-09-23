<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ad70058c2d21 -->

# Catalog::tabelize

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::tabelize(const TabelizePolicy& policy = {}) const -> pluss::table::DataFrame
```

```cpp
struct TabelizePolicy {
  Align align{Align::Inner};
  AlignBy align_by{AlignBy::Label};
  FillPolicy fill{FillPolicy::Null};
  std::optional<double> fill_scalar{std::nullopt};
  DTypePolicy dtype{DTypePolicy::BestEffort};
  CollisionPolicy collision{CollisionPolicy::Error};
  std::optional<std::string> anchor{std::nullopt};
  bool include_label_feature{false};
  std::string label_feature{"label"};
};
```

## What It Does

Converts the catalog into a `pluss::table::DataFrame`, one feature per stream, aligning
all streams onto a single shared row axis first. This is the bridge every `save_*()`
method uses to turn a possibly-ragged `Catalog` into a strictly rectangular
`DataFrame`. [`DataFrame::kindelize()`](../dataframe/kindelize.md) is the inverse
direction.

```{important} The default Align::Inner truncates a ragged catalog to its shortest stream
With the default policy, streams of different lengths are aligned to their **shortest**
common length — extra rows in longer streams are silently dropped. If you want every row
preserved instead (with gaps filled), set `policy.align = Align::Outer`, which pads
shorter streams instead of truncating longer ones. See Example 2.
```

## Parameters

**policy** (`const TabelizePolicy&`, optional)
    `align` controls truncate-vs-pad behavior on a ragged catalog (`Inner` truncates to
    the shortest stream, the default; `Outer` pads to the longest). `fill` (default
    `Null`) controls what fills the gaps under `Outer` alignment. `include_label_feature`
    (default `false`) additionally adds the row-index axis as its own feature, named
    `label_feature` (default `"label"`); `collision` decides what happens if that name
    already exists as a stream.

## Returns

`pluss::table::DataFrame`
    A new frame with one feature per stream, aligned to a single row axis.

## Examples

Example 1. Tabelize a catalog whose streams already share the same length.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto frame = ct.tabelize();
std::cout << frame;
```

### Output

```text
===================
     | id    city  
-------------------
0    | 1     Accra 
1    | 2     Tema  
2    | 3     Kumasi
-------------------
[3 cs x 2 feat]
```

Example 2. A ragged catalog, with the default `Align::Inner` — truncated to the shortest
stream's length (`2`), losing `a`'s third row.

```cpp
auto ragged = pluss::kindle::Catalog({
    "a"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "b"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});

auto frame = ragged.tabelize();
std::cout << frame;
```

### Output

```text
=================
     | a     b   
-----------------
0    | 1     10  
1    | 2     20  
-----------------
[2 cs x 2 feat]
```

Example 3. The same ragged catalog with `Align::Outer` — every row is preserved, with the
shorter stream's gap filled `NULL`.

```cpp
auto ragged = pluss::kindle::Catalog({
    "a"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "b"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});

pluss::kindle::TabelizePolicy policy;
policy.align = pluss::kindle::Align::Outer;

auto frame = ragged.tabelize(policy);
std::cout << frame;
```

### Output

```text
=================
     | a     b   
-----------------
0    | 1     10  
1    | 2     20  
2    | 3     NULL
-----------------
[3 cs x 2 feat]
```

Example 4. `include_label_feature = true` adds the row-index axis as its own feature.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

pluss::kindle::TabelizePolicy policy;
policy.include_label_feature = true;

auto frame = ct.tabelize(policy);
std::cout << frame;
```

### Output

```text
==========================
     | id    city    label
--------------------------
0    | 1     Accra   0    
1    | 2     Tema    1    
2    | 3     Kumasi  2    
--------------------------
[3 cs x 3 feat]
```

## Notes

- `Align::Inner` (the default) truncates a ragged catalog to its shortest stream — see
  the important callout above. Use `Align::Outer` to preserve every row instead.
- An empty catalog (zero streams) tabelizes to an empty, zero-feature `DataFrame` rather
  than throwing.
- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.

## See Also

- [`DataFrame::kindelize`](../dataframe/kindelize.md): the inverse direction,
  `DataFrame` → `Catalog`.
- [`Catalog::save_csv`](save_csv.md) (and the other `save_*()` methods): use an
  outer-align variant of this same mechanism internally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="tail.html">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>
