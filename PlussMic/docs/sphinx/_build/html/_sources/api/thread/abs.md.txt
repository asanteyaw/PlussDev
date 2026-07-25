<!-- pluss-hand-authored -->

# Thread::abs

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::abs() const -> Thread
```

## What It Does

Returns the elementwise absolute value of a `Series`-kind thread — strips the sign,
leaving magnitude. Requires the thread to be `Series`-kind.

## Parameters

`None`

## Returns

`Thread`
    A new `Series`-kind thread, same length as `*this`, holding each element's
    absolute value.

## Examples

Example 1. Absolute value strips sign; zero and positive values are unchanged.

```cpp
using namespace pluss::table;

auto deltas = Thread(pluss::make_series({-2.5, 0.0, 3.5}));
auto out = deltas.abs();
std::cout << out;
```

### Output

```text
0  2.5
1  0
2  3.5

ptype: Series
```

Example 2. A non-`Series` thread throws.

```cpp
auto notes = Thread(pluss::make_note({"a", "b"}));
try {
  auto bad = notes.abs();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::abs is only valid for Series payload
```

## Notes

- Requires the calling thread to be `Series`-kind.
- A `NULL` cell on input stays `NULL` on output.

## See Also

- [`Thread::clip`](clip.md): cap values into a range, rather than stripping sign.
- [`Thread::pow`](pow.md): more general elementwise numeric transform.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="add.html">
    <span class="label">Next</span>
    <span class="title">add</span>
  </a>
</div>
