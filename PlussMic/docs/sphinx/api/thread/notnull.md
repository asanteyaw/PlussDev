<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2be009f73669 -->

# Thread::notnull

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::notnull() const -> Thread
```

## What It Does

Returns a `Bool`-typed `Series` thread, same length as `*this`, marking `true`
wherever the source thread is valid (not `NULL`) and `false` wherever it's `NULL` —
the exact logical inverse of [`isnull()`](isnull.md). Works on every payload kind,
with the same validity-mask-only semantics (and the same `Timeline`/`TimeIndex`
internal-validity folding) described on that page.

## Parameters

`None`

## Returns

`Thread`
    A new `Bool`-typed `Series` thread, same length as `*this`.

## Examples

Example 1. `notnull()` and `isnull()` are exact opposites, elementwise.

```cpp
using namespace pluss::table;

auto numbers = Thread(pluss::make_series({1.0, 2.0, 3.0}),
    pluss::make_series(std::vector<bool>{true, false, true}));
std::cout << numbers.notnull() << "\n";
std::cout << numbers.isnull();
```

### Output

```text
0  true
1  false
2  true

ptype: Series
0  false
1  true
2  false

ptype: Series
```

## Notes

- The exact logical inverse of [`isnull()`](isnull.md); see that page for the
  validity-mask-only semantics and the `Timeline`/`TimeIndex` nuance.
- A thread with no validity mask at all reports `true` at every position.

## See Also

- [`Thread::isnull`](isnull.md): the logical inverse.
- [`Thread::dropnull`](dropnull.md): remove `NULL` rows using the same underlying
  check.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normalize.html">
    <span class="label">Previous</span>
    <span class="title">normalize</span>
  </a>
  <a class="next-link" href="nt.html">
    <span class="label">Next</span>
    <span class="title">nt</span>
  </a>
</div>
