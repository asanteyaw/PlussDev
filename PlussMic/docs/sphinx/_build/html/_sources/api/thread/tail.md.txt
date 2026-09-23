<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 94eed779a0c0 -->

# Thread::tail

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::tail(std::int64_t n = 5, bool render = false) const -> ThreadView
```

## What It Does

Returns the last `n` entries of the thread as a `ThreadView` — see
[`head()`](head.md) for the shared `ThreadView` mechanics (it's a full `Thread`
subclass, renders identically). `n` is clamped into `[0, size()]`, same as
`head()` — never throws for an out-of-range `n`.

## Parameters

**n** (`std::int64_t`, default `5`)
    Number of trailing entries to keep. Clamped into `[0, size()]`.

**render** (`bool`, default `false`)
    See [`head()`](head.md)'s parameter description — auto-render-on-scope-exit
    only, no effect on explicit stream insertion.

## Returns

`ThreadView`
    A view over the last `n` entries (or every entry, if `n >= size()`), in their
    original order.

## Examples

Example 1. The last two entries, in original order (not reversed).

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto out = scores.tail(2);
std::cout << out;
```

### Output

```text
0  30
1  40

ptype: Series
```

Row indices in the printed output restart at `0` for the view — they don't carry
the original positions `2`/`3` forward.

Example 2. `n` larger than `size()` clamps.

```cpp
auto out2 = scores.tail(100);
std::cout << out2;
```

### Output

```text
0  10
1  20
2  30
3  40

ptype: Series
```

## Notes

- `n` is clamped into `[0, size()]`, never throws.
- Printed row indices restart at `0` for the returned view — the original row
  positions aren't preserved in the display.

## See Also

- [`Thread::head`](head.md): the leading-entries counterpart.
- [`Thread::view`](view.md): an arbitrary `[start, stop)` window.
- [`Thread::shift`](shift.md): move values by position without dropping any rows.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="take.html">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>
