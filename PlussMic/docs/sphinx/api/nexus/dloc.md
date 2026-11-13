<!-- pluss-hand-authored -->

# Nexus::dloc

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
DataFrame Nexus::dloc(std::int64_t index) const
```

## What It Does

An exact alias for the positional overload of [`frame()`](frame.md) —
`dloc(index)` calls `frame(index)` internally and returns the identical result,
negative-index wraparound included. The name mirrors
[`Atlas::dloc`](../../atlas_api_reference.md)'s identically-named, identically-behaved
method, since both `Nexus` and `Atlas` are depth-stacked collections addressed by
position along that "depth" axis.

## Parameters

**index** (`std::int64_t`)

The zero-based position to fetch. Negative values count back from the end. Throws if
out of bounds.

## Returns

`DataFrame`
    The table at `index` — identical to [`frame(index)`](frame.md).

## Examples

Example 1. `dloc()` and the positional `frame()` overload always agree.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", pluss::make_series({1.0, 2.0, 3.0})},
    {"name",        pluss::make_note({"Ada", "Bola", "Chen"})},
});

auto orders = DataFrame({
    {"order_id",    pluss::make_series({100.0, 101.0, 102.0})},
    {"customer_id", pluss::make_series({1.0, 2.0, 2.0})},
    {"amount",      pluss::make_series({50.0, 75.0, 20.0})},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
});

std::cout << (biz.dloc(1).equals(biz.frame(1)));
```

### Output

```text
true
```

## Notes

- Purely a naming alternative to the positional overload of [`frame()`](frame.md) —
  see that page for the full breakdown, including negative-index behavior and the
  out-of-bounds throw message.
- There is no name-based `dloc()` overload — for a name-based lookup, use
  [`frame(key)`](frame.md) or [`get(key)`](get.md).

## See Also

- [`Nexus::frame`](frame.md): the canonical method, with both the name-based and
  positional overloads.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="detect_orphans.html">
    <span class="label">Previous</span>
    <span class="title">detect_orphans</span>
  </a>
  <a class="next-link" href="drop.html">
    <span class="label">Next</span>
    <span class="title">drop</span>
  </a>
</div>
