<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e9493eadfb5e -->

# Slate::rmul

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate Slate::rmul(double other) const
Slate Slate::rmul(const Slate& other) const
```

## What It Does

The reflected form of [`mul()`](mul.md). Multiplication is commutative, so `rmul()`
and `mul()` always agree — including for `Note`-slate repetition, since the
repetition special case never inspects operand order.

## Parameters

**other** (`double` or `const Slate&`)

Same shape as [`mul()`](mul.md).

## Returns

`Slate`
    Identical to what `mul()` would produce for the same arguments.

## Examples

Example 1. `rmul()` and `mul()` agree, for numbers and for `Note` repetition alike.

```cpp
using namespace pluss::table;

auto prices = Slate(pluss::make_series({10.0, 20.0, 30.0}));
auto word = Slate(pluss::make_note({"ab", "xy", "q"}));

std::cout << prices.rmul(2.0) << "\n" << word.rmul(3.0);
```

### Output

```text
0  20
1  40
2  60

ptype: Series
0  ababab
1  xyxyxy
2  qqq

ptype: Note
```

Both match [`prices.mul(2.0)`](mul.md) and [`word.mul(3.0)`](mul.md) exactly.

## Notes

- `rmul()` exists purely for API symmetry with the rest of the reflected family —
  there is no case where it behaves differently from `mul()`.

## See Also

- [`Slate::mul`](mul.md): the forward form, and the full documentation of the
  `Note`-repetition special case.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rmod.html">
    <span class="label">Previous</span>
    <span class="title">rmod</span>
  </a>
  <a class="next-link" href="ror.html">
    <span class="label">Next</span>
    <span class="title">ror</span>
  </a>
</div>
