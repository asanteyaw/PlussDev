<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 849a31a3d98c -->

# Shelf::ror

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
Shelf Shelf::ror(const Shelf& other) const
```

## What It Does

The reflected form of [`lor()`](lor.md). Logical OR is commutative, so `ror()` and
`lor()` always compute the same boolean values — but, exactly like
[`rand()`](rand.md) versus [`land()`](land.md), `ror()`'s result carries display
labels from `other` instead of from `*this`. See [`rand()`](rand.md) for the full
explanation and a worked example of this label-carry difference.

## Parameters

**other** (`const Shelf&`)

Same shape as [`lor()`](lor.md).

## Returns

`Shelf`
    The same values [`lor()`](lor.md) would produce for the same arguments, with
    display labels carried from `other` instead of `*this`.

## Examples

Example 1. Same values as `lor()`.

```cpp
using namespace pluss::table;

auto left = Shelf(pluss::make_series({true, false, false}, pluss::ScalarType::Bool));
auto right = Shelf(pluss::make_series({false, false, true}, pluss::ScalarType::Bool));

auto out = left.ror(right);
std::cout << out;
```

### Output

```text
0  true
1  false
2  true

ptype: Series
```

Matches [`left.lor(right)`](lor.md) exactly here, since neither shelf has display
labels set — the label-carry difference between `lor()`/`ror()` only becomes visible
once labels are involved. See [`rand()`](rand.md) for a worked example with labels.

## Notes

- See [`rand()`](rand.md) for the full explanation of this label-carry difference,
  which applies identically here.

## See Also

- [`Shelf::lor`](lor.md): the forward form.
- [`Shelf::rand`](rand.md): the fuller explanation of this reflected-family
  label-carry behavior.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rmul.html">
    <span class="label">Previous</span>
    <span class="title">rmul</span>
  </a>
  <a class="next-link" href="round.html">
    <span class="label">Next</span>
    <span class="title">round</span>
  </a>
</div>
