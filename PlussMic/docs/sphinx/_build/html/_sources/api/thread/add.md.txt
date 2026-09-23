<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 53b236672f13 -->

# Thread::add

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::add(double other) const -> Thread
Thread::add(const Thread& other) const -> Thread
```

## What It Does

Elementwise addition. The `double` overload adds the same scalar to every value; the
`Thread` overload adds two threads together **positionally** — element `i` of the
result is `(*this)[i] + other[i]` — and requires both threads to have the same
length. There is no case-label alignment the way [`DataFrame`](../dataframe/add.md)'s
`AlignOptions` overload has; a length mismatch throws rather than padding with
`NULL`.

`add()` normally requires a numeric `Series`-backed thread, with one special case:
two `Note` (string) threads support `add()` too, and it means **string
concatenation**, not addition — `"north-".add("store")` behaves like `"north-" +
"store"`, not a type error.

```{important} add() is the canonical arithmetic-family reference
The rest of Thread's arithmetic methods (`sub`, `mul`, `div`, `truediv`, `floordiv`,
`mod`, `pow`, and their reflected `r`-prefixed forms) all share this exact overload
shape, positional-matching rule, and length-mismatch behavior. Their own pages focus
on what's different about each operator; this page covers the shared mechanics once.
```

## Parameters

**other** (`double` or `const Thread&`)

The value(s) to add. For the `Thread` overload, must be the same length as `*this`
(or a `Note`-kind thread, for concatenation).

## Returns

`Thread`
    A new thread, same length as `*this`, holding the elementwise sum (or, for two
    `Note` threads, the elementwise concatenation).

## Examples

Example 1. Add a scalar to every value.

```cpp
using namespace pluss::table;

auto prices = Thread(pluss::make_series({10.0, 20.0, 30.0}));
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

Example 2. Add two threads positionally.

```cpp
auto boost = Thread(pluss::make_series({2.0, 4.0, 5.0}));
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
purely positional, with no reference to any case labels either thread might carry.

Example 3. Two `Note` (string) threads concatenate instead of erroring.

```cpp
auto prefix = Thread(pluss::make_note({"north-", "south-", "east-"}));
auto base_name = Thread(pluss::make_note({"store", "hub", "depot"}));
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
auto too_short = Thread(pluss::make_series({1.0, 2.0}));
try {
  auto bad = prices.add(too_short);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::add(Thread): positional operation requires identical lengths
```

## Notes

- The `Thread` overload's error message is prefixed with the exact call site
  (`Thread::add(Thread)`, `Thread::sub(Thread)`, and so on) — every method in this
  family reports its own name, not a shared generic message.
- Mixing incompatible payload kinds (for example, adding a numeric `Series` thread to
  a `Note` thread) throws `"Thread::add(Thread): unsupported payload combination for
  operator '+' (Series vs Note)"` — `add()` only accepts two numeric threads or two
  `Note` threads, never a mix.
- If the calling thread carries display/case labels (set via `label().set()`), the
  result carries those same labels forward; `other`'s labels (if any) are never
  used.
- [`radd()`](radd.md) is the reflected form — identical to `add()` for numbers
  (addition is commutative), but the operand order flips for `Note` concatenation.

## See Also

- [`Thread::radd`](radd.md): the reflected form.
- [`Thread::sub`](sub.md), [`Thread::mul`](mul.md), [`Thread::div`](div.md): the rest
  of the arithmetic family, sharing this same overload shape.
- [`DataFrame::add`](../dataframe/add.md): the multi-feature counterpart, which adds
  an `AlignOptions`-based case-label-alignment overload that `Thread::add()` has no
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
