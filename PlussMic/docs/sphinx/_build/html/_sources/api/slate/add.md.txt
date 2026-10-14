<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f801e1d45150 -->

# Slate::add

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate::add(double other) const -> Slate
Slate::add(const Slate& other) const -> Slate
```

## What It Does

Elementwise addition. The `double` overload adds the same scalar to every value; the
`Slate` overload adds two slates together **positionally** — element `i` of the
result is `(*this)[i] + other[i]` — and requires both slates to have the same
length. There is no case-label alignment the way [`DataFrame`](../dataframe/add.md)'s
`AlignOptions` overload has; a length mismatch throws rather than padding with
`NULL`.

`add()` normally requires a numeric `Series`-backed slate, with one special case:
two `Note` (string) slates support `add()` too, and it means **string
concatenation**, not addition — `"north-".add("store")` behaves like `"north-" +
"store"`, not a type error.

```{important} add() is the canonical arithmetic-family reference
The rest of Slate's arithmetic methods (`sub`, `mul`, `div`, `truediv`, `floordiv`,
`mod`, `pow`, and their reflected `r`-prefixed forms) all share this exact overload
shape, positional-matching rule, and length-mismatch behavior. Their own pages focus
on what's different about each operator; this page covers the shared mechanics once.
```

## Parameters

**other** (`double` or `const Slate&`)

The value(s) to add. For the `Slate` overload, must be the same length as `*this`
(or a `Note`-kind slate, for concatenation).

## Returns

`Slate`
    A new slate, same length as `*this`, holding the elementwise sum (or, for two
    `Note` slates, the elementwise concatenation).

## Examples

Example 1. Add a scalar to every value.

```cpp
using namespace pluss::table;

auto prices = Slate(pluss::make_series({10.0, 20.0, 30.0}));
auto out = prices.add(5.0);
std::cout << out;
```

### Output

```text
0  15
1  25
2  35

ptype: Series
```

Example 2. Add two slates positionally.

```cpp
auto boost = Slate(pluss::make_series({2.0, 4.0, 5.0}));
auto out_pair = prices.add(boost);
std::cout << out_pair;
```

### Output

```text
0  12
1  24
2  35

ptype: Series
```

Position `0` adds `10 + 2`, position `1` adds `20 + 4`, position `2` adds `30 + 5` —
purely positional, with no reference to any case labels either slate might carry.

Example 3. Two `Note` (string) slates concatenate instead of erroring.

```cpp
auto prefix = Slate(pluss::make_note({"north-", "south-", "east-"}));
auto base_name = Slate(pluss::make_note({"store", "hub", "depot"}));
auto out_concat = prefix.add(base_name);
std::cout << out_concat;
```

### Output

```text
0  north-store
1  south-hub
2  east-depot

ptype: Note
```

This is the one payload combination `add()` accepts beyond plain numbers — string
concatenation, order-sensitive (`prefix.add(base_name)` puts `prefix`'s value first
at each position). See [`radd()`](radd.md) for how the reflected form reverses this
order.

Example 4. A length mismatch throws rather than aligning or padding.

```cpp
auto too_short = Slate(pluss::make_series({1.0, 2.0}));
try {
  auto bad = prices.add(too_short);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::add(Slate): positional operation requires identical lengths
```

## Notes

- The `Slate` overload's error message is prefixed with the exact call site
  (`Slate::add(Slate)`, `Slate::sub(Slate)`, and so on) — every method in this
  family reports its own name, not a shared generic message.
- Mixing incompatible payload kinds (for example, adding a numeric `Series` slate to
  a `Note` slate) throws `"Slate::add(Slate): unsupported payload combination for
  operator '+' (Series vs Note)"` — `add()` only accepts two numeric slates or two
  `Note` slates, never a mix.
- If the calling slate carries display/case labels (set via `label().set()`), the
  result carries those same labels forward; `other`'s labels (if any) are never
  used.
- [`radd()`](radd.md) is the reflected form — identical to `add()` for numbers
  (addition is commutative), but the operand order flips for `Note` concatenation.

## See Also

- [`Slate::radd`](radd.md): the reflected form.
- [`Slate::sub`](sub.md), [`Slate::mul`](mul.md), [`Slate::div`](div.md): the rest
  of the arithmetic family, sharing this same overload shape.
- [`DataFrame::add`](../dataframe/add.md): the multi-feature counterpart, which adds
  an `AlignOptions`-based case-label-alignment overload that `Slate::add()` has no
  equivalent of.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="abs.html">
    <span class="label">Previous</span>
    <span class="title">abs</span>
  </a>
  <a class="next-link" href="align.html">
    <span class="label">Next</span>
    <span class="title">align</span>
  </a>
</div>
