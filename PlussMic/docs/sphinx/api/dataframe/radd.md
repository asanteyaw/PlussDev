<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 26b0360b1cc8 -->

# DataFrame::radd

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame DataFrame::radd(double other) const
```

## What It Does

The reflected (right-hand) form of [`add()`](add.md): computes `other + frame` instead
of `frame + other`. `radd()`, [`rsub()`](rsub.md), `rmul`, `rdiv`, `rtruediv`,
`rfloordiv`, `rmod`, and `rpow` are the reflected counterparts of the eight forward
arithmetic methods, all scalar-only (`double other`) — there is no reflected
`DataFrame`-to-`DataFrame` or `Slate` form, since swapping which side is the "frame"
is unambiguous once a scalar is the other operand.

For addition specifically, `frame.add(other)` and `frame.radd(other)` produce the
*same* result, since addition is commutative — `radd()` mainly exists for symmetry
with the rest of the reflected family (where operand order genuinely changes the
result, such as [`rsub()`](rsub.md)) and for call sites where "the scalar comes first"
reads more naturally.

## Parameters

**other** (`double`)

The scalar added to every numeric feature on the frame.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature increased by
    `other`. Non-numeric features are left unchanged.

## Examples

Example 1. `radd()` and `add()` produce the same result for addition.

```cpp
using namespace pluss::table;

auto values = DataFrame({
    {"x", pluss::make_series({2.0, 3.0, 4.0})},
}, pluss::make_note({"a", "b", "c"}));

auto out = values.radd(10.0);
std::cout << out;
```

### Output

```text
===========
     | x   
-----------
a    | 12  
b    | 13  
c    | 14  
-----------
[3 cs x 1 feat]
```

`values.add(10.0)` would produce this identical result — `10 + x` and `x + 10` are the
same number.

## Notes

- All eight reflected methods (`radd`, `rsub`, `rmul`, `rdiv`, `rtruediv`,
  `rfloordiv`, `rmod`, `rpow`) only accept a scalar `double`; there is no reflected
  `DataFrame` or `Slate` overload.
- For commutative operations (`radd`, `rmul`), the reflected and forward forms always
  agree. For non-commutative operations (`rsub`, `rdiv`, `rtruediv`, `rfloordiv`,
  `rmod`, `rpow`), the reflected form computes a genuinely different result — see
  [`rsub()`](rsub.md) for a worked example.

## See Also

- [`DataFrame::add`](add.md): the forward form, and the full explanation of this
  arithmetic family's shared behavior.
- [`DataFrame::rsub`](rsub.md): the reflected subtraction, where operand order
  actually changes the result.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="query.html">
    <span class="label">Previous</span>
    <span class="title">query</span>
  </a>
  <a class="next-link" href="rand.html">
    <span class="label">Next</span>
    <span class="title">rand</span>
  </a>
</div>
