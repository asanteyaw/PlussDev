<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 46b790c052de -->

# Catalog::to_table

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::to_table(const TabelizePolicy& policy = {}, bool render = false) const -> pluss::table::DataFrame
```

## What It Does

Converts the catalog into a `pluss::table::DataFrame` — a thin wrapper over
[`tabelize()`](tabelize.md) that additionally prints the resulting `DataFrame` to
`std::cout` when `render = true`. The printed rendering (a `DataFrame`-style table with a
`[N cs x M feat]` footer) is different from `Catalog`'s own ASCII-table rendering used by
[`to_string()`](to_string.md)/[`show()`](show.md).

## Parameters

**policy** (`const TabelizePolicy&`, optional)
    The same alignment policy [`tabelize()`](tabelize.md) accepts.

**render** (`bool`, default `false`)
    When `true`, additionally prints the resulting `DataFrame` to `std::cout` as a side
    effect, in addition to returning it.

## Returns

`pluss::table::DataFrame`
    The tabelized frame.

## Examples

Example 1. Convert to a `DataFrame` without printing.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto frame = ct.to_table();
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

Example 2. `render = true` prints the same table to `std::cout` immediately, as a side
effect, in addition to returning the `DataFrame`.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto frame = ct.to_table({}, true);
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

Note this output comes from the `render = true` side effect itself — nothing in the
example code above explicitly prints `frame`.

## Notes

- A thin wrapper over [`tabelize()`](tabelize.md), with an optional print side effect.
- The rendering is `DataFrame`-style (`[N cs x M feat]` footer), distinct from `Catalog`'s
  own rendering used by [`to_string()`](to_string.md)/[`show()`](show.md).

## See Also

- [`Catalog::tabelize`](tabelize.md): the underlying conversion, without the optional
  print side effect.
- [`Catalog::kindelize`](kindelize.md): the inverse direction, `DataFrame` → `Catalog`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="transform.html">
    <span class="label">Next</span>
    <span class="title">transform</span>
  </a>
</div>
