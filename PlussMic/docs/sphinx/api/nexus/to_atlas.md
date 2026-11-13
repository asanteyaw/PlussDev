<!-- pluss-hand-authored -->

# Nexus::to_atlas

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Atlas Nexus::to_atlas(std::initializer_list<std::string_view> keys, std::optional<Slate> labels = std::nullopt) const
Atlas Nexus::to_atlas(pluss::msharp::RangeView::Of<std::string_view> keys, std::optional<Slate> labels = std::nullopt) const
template <detail::StringViewNameRange NameRange>
Atlas Nexus::to_atlas(const NameRange& keys, std::optional<Slate> labels = std::nullopt) const
```

## What It Does

Collects selected workspace tables back into an [`Atlas`](../../atlas_api_reference) — the
inverse of [`from_atlas()`](from_atlas.md). Since an `Atlas` requires every frame in the
stack to share identical column names and order, `keys` must name tables that are actually
same-shaped; mixing tables with different schemas throws (from `Atlas`'s own constructor,
not from `to_atlas()` itself).

## Parameters

**keys** (a range of `std::string_view` — a brace literal, a named container
such as `std::vector<std::string_view>`/`std::vector<std::string>`, or any
other range whose elements convert to `std::string_view`)
    The tables to collect, in iteration order. Same
    validation as [`select()`](select.md)'s key list: non-empty, unique, and each must
    already exist in the workspace.

```{note} `pluss::msharp::RangeView` is never spelled out in these examples
See [`Nexus::select`](select.md)'s own note on this — the same
three-overload pattern (brace literal / named `RangeView`-convertible
container / any other qualifying range) applies here identically, including
the same "iteration order becomes output order" rule (an ordered container
like `std::set` iterates sorted, not in insertion order).
```

**labels** (`std::optional<Slate>`, default `std::nullopt`)
    Axis-0 labels for the resulting atlas. Left at the default, `to_atlas()` uses the
    requested `keys` themselves as the labels (as Note-kind text) — so the atlas remembers
    which table each layer came from even without you supplying labels explicitly.

## Returns

`Atlas`
    A stack containing the requested tables' frames, in request order.

## Examples

Example 1. Collecting two same-shaped tables into an atlas, using the default (key-derived)
labels.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto orders_q1 = DataFrame({
    {"order_id", Slate(pluss::make_series({101.0, 102.0}))},
    {"amount",   Slate(pluss::make_series({50.0, 75.0}))},
});
auto orders_q2 = DataFrame({
    {"order_id", Slate(pluss::make_series({201.0, 202.0}))},
    {"amount",   Slate(pluss::make_series({40.0, 60.0}))},
});

auto biz = Nexus({
    {"orders_q1", orders_q1},
    {"orders_q2", orders_q2},
});

auto atlas = biz.to_atlas({"orders_q1", "orders_q2"});
std::cout << atlas.depth();
```

### Output

```text
2
```

Example 2. The same request via named containers — a
`std::vector<std::string_view>` (the `RangeView` overload) and a
`std::vector<std::string>` (the templated overload).

```cpp
std::vector<std::string_view> as_view = {"orders_q1", "orders_q2"};
std::vector<std::string> as_owned = {"orders_q1", "orders_q2"};

std::cout << biz.to_atlas(as_view).depth() << " "
          << biz.to_atlas(as_owned).depth();
```

### Output

```text
2 2
```

## Notes

- Every named table must share identical feature names and order — `Atlas`'s own
  constructor throws `"Atlas: all frames must have identical column names/order"`
  otherwise.
- Without explicit `labels`, the requested `keys` themselves become the atlas's axis-0
  labels, so [`from_atlas()`](from_atlas.md) run on the result recovers the original table
  names automatically.

## See Also

- [`Nexus::from_atlas`](from_atlas.md): the inverse — build a `Nexus` from an `Atlas`.
- [`Nexus::select`](select.md): the same key-list validation, for the table-subsetting
  (non-`Atlas`) case.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
