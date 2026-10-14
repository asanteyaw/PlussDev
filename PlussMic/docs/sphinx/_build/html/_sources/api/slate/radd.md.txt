<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 56dcefbb3f5d -->

# Slate::radd

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate::radd(double other) const -> Slate
Slate::radd(const Slate& other) const -> Slate
```

## What It Does

The reflected form of [`add()`](add.md). For numbers, addition is commutative, so
`radd()` and `add()` produce identical results in every observable way. The
distinction only matters for `Note` (string) slates, where `add()` means
concatenation and concatenation order depends on which operand comes first —
`radd()` reverses that order.

## Parameters

**other** (`double` or `const Slate&`)

Same shape as [`add()`](add.md).

## Returns

`Slate`
    For numbers, identical to what `add()` would produce for the same arguments. For
    two `Note` slates, the concatenation with the operand order reversed.

## Examples

Example 1. For a scalar, `radd()` and `add()` agree exactly.

```cpp
using namespace pluss::table;

auto prices = Slate(pluss::make_series({10.0, 20.0, 30.0}));
auto out = prices.radd(5.0);
std::cout << out;
```

### Output

```text
0  15
1  25
2  35

ptype: Series
```

Same values as [`prices.add(5.0)`](add.md) — addition doesn't care about order.

Example 2. For `Note` slates, `radd()` concatenates in the opposite order from
`add()`.

```cpp
auto prefix = Slate(pluss::make_note({"north-", "south-", "east-"}));
auto base_name = Slate(pluss::make_note({"store", "hub", "depot"}));

auto forward = prefix.add(base_name);
auto reflected = prefix.radd(base_name);
std::cout << forward << "\n" << reflected;
```

### Output

```text
0  north-store
1  south-hub
2  east-depot

ptype: Note
0  storenorth-
1  hubsouth-
2  depoteast-

ptype: Note
```

`prefix.add(base_name)` puts `prefix`'s text first at each position;
`prefix.radd(base_name)` is equivalent to `base_name.add(prefix)` — `base_name`'s
text comes first instead.

## Notes

- For numeric slates, `radd()` exists purely for API symmetry with the rest of the
  reflected family (`rsub`, `rmul`, and so on) — there is no behavioral reason to
  prefer it over `add()`.
- See [`add()`](add.md) for the shared mechanics every arithmetic method in this
  family follows: positional matching, the length-mismatch throw, and the
  `Note`-concatenation special case.

## See Also

- [`Slate::add`](add.md): the forward form, and the canonical reference for this
  whole arithmetic family's shared mechanics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="put.html">
    <span class="label">Previous</span>
    <span class="title">put</span>
  </a>
  <a class="next-link" href="rand.html">
    <span class="label">Next</span>
    <span class="title">rand</span>
  </a>
</div>
