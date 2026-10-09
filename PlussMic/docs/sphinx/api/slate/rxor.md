<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d2bbf854f9cd -->

# Slate::rxor

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::rxor(const Slate& other) const -> Slate
```

## What It Does

The reflected form of [`lxor()`](lxor.md). Logical XOR is commutative, so `rxor()`
and `lxor()` always compute the same boolean values — but, exactly like
[`rand()`](rand.md) versus [`land()`](land.md), `rxor()`'s result carries display
labels from `other` instead of from `*this`. See [`rand()`](rand.md) for the full
explanation and a worked example of this label-carry difference.

## Parameters

**other** (`const Slate&`)

Same shape as [`lxor()`](lxor.md).

## Returns

`Slate`
    The same values [`lxor()`](lxor.md) would produce for the same arguments, with
    display labels carried from `other` instead of `*this`.

## Examples

Example 1. Same values as `lxor()`.

```cpp
using namespace pluss::table;

auto left = Slate(pluss::make_series({true, false, true}, pluss::ScalarType::Bool));
auto right = Slate(pluss::make_series({true, true, false}, pluss::ScalarType::Bool));

auto out = left.rxor(right);
std::cout << out;
```

### Output

```text
0  false
1  true
2  true

ptype: Series
```

Matches [`left.lxor(right)`](lxor.md) exactly here — the label-carry difference only
becomes visible once display labels are involved. See [`rand()`](rand.md) for a
worked example with labels.

## Notes

- See [`rand()`](rand.md) for the full explanation of this label-carry difference,
  which applies identically here.

## See Also

- [`Slate::lxor`](lxor.md): the forward form.
- [`Slate::rand`](rand.md): the fuller explanation of this reflected-family
  label-carry behavior.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rtruediv.html">
    <span class="label">Previous</span>
    <span class="title">rtruediv</span>
  </a>
  <a class="next-link" href="sample.html">
    <span class="label">Next</span>
    <span class="title">sample</span>
  </a>
</div>
