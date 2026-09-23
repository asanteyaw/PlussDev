<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4e518903d766 -->

# Thread::exp

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::exp() const -> Thread
```

## What It Does

Applies the elementwise natural exponential (`e^x`) — the inverse of
[`log()`](log.md). Requires the thread to be `Series`-kind. A non-floating scalar
type (integer, bool) is promoted to `Float64` before the exponential is computed.

## Parameters

`None`

## Returns

`Thread`
    A new `Series`-kind thread, same length as `*this`, holding `exp(x)` for each
    element (promoted to a floating scalar type if the input was integral).

## Examples

Example 1. Exponentiate a small numeric thread.

```cpp
using namespace pluss::table;

auto values = Thread(pluss::make_series({0.0, 1.0, 2.0}));
auto out = values.exp();
std::cout << out;
```

### Output

```text
0  1
1  2.718281828
2  7.389056099

ptype: Series
```

Example 2. `log()` and `exp()` round-trip back to the original values (subject to
ordinary floating-point precision).

```cpp
auto original = Thread(pluss::make_series({5.0, 12.5, 100.0}));
auto round_trip = original.log().exp();
std::cout << round_trip;
```

### Output

```text
0  5
1  12.5
2  100

ptype: Series
```

Example 3. A non-`Series` thread throws.

```cpp
auto notes = Thread(pluss::make_note({"a", "b"}));
try {
  auto bad = notes.exp();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::exp is only valid for Series payload
```

## Notes

- Requires the calling thread to be `Series`-kind.
- Integer/bool input is promoted to `Float64` automatically before computing.
- A `NULL` cell on input stays `NULL` on output.

## See Also

- [`Thread::log`](log.md): the inverse transform.
- [`Thread::pow`](pow.md): general exponentiation, for bases other than *e*.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eq.html">
    <span class="label">Previous</span>
    <span class="title">eq</span>
  </a>
  <a class="next-link" href="extract.html">
    <span class="label">Next</span>
    <span class="title">extract</span>
  </a>
</div>
