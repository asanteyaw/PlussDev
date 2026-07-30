<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a5a354513084 -->

# Thread::filter

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::filter(const pluss::Series& bool_mask) const -> Thread
```

## What It Does

Keeps only the entries where `bool_mask` is `true`, dropping the rest — the classic
boolean-mask row filter, built on top of [`take()`](take.md) (`filter()` converts
the mask to the list of `true` positions, then takes them in order). `bool_mask`
must be a 1D, `Bool`-scalar-typed `pluss::Series` with exactly `size()` elements.

## Parameters

**bool_mask** (`const pluss::Series&`)
    A `Bool`-typed, 1D `Series` the same length as `*this`. Each `true` position is
    kept, in original order; every `false` position is dropped.

## Returns

`Thread`
    A new thread holding only the entries where `bool_mask` was `true`, in their
    original relative order.

## Examples

Example 1. Keep only the entries above a threshold, via a mask built from
[`gt()`](gt.md).

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 25.0, 30.0, 15.0}));
auto mask = scores.gt(20.0).as_series();
auto out = scores.filter(mask);
std::cout << out;
```

### Output

```text
0  25
1  30

ptype: Series
```

Example 2. A hand-built mask works the same way.

```cpp
auto mask2 = pluss::make_series(std::vector<bool>{true, false, true, false});
auto out2 = scores.filter(mask2);
std::cout << out2;
```

### Output

```text
0  10
1  30

ptype: Series
```

Example 3. A length-mismatched mask throws.

```cpp
try {
  auto bad_mask = pluss::make_series(std::vector<bool>{true, false});
  auto bad = scores.filter(bad_mask);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::filter: bool mask length mismatch
```

## Notes

- `bool_mask` must be exactly `size()` elements long, 1D, and `Bool`-typed, or the
  call throws (`"Thread::filter: bool mask length mismatch"`,
  `"...bool mask must be 1D"`, or `"...bool mask dtype must be Bool"`, depending on
  which check fails).
- Built on [`take()`](take.md) — the true-position indices are gathered and passed
  straight through.
- Preserves relative order of the kept entries.

## See Also

- [`Thread::take`](take.md): the general gather this method is built on.
- [`Thread::gt`](gt.md) / [`Thread::eq`](eq.md) / other comparisons: the usual
  source of the boolean mask passed here.
- [`Thread::dropnull`](dropnull.md): a specialized filter for `NULL` removal.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnull_.html">
    <span class="label">Previous</span>
    <span class="title">fillnull_</span>
  </a>
  <a class="next-link" href="floordiv.html">
    <span class="label">Next</span>
    <span class="title">floordiv</span>
  </a>
</div>
