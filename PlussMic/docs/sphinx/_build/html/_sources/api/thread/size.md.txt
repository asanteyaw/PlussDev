<!-- pluss-hand-authored -->

# Thread::size

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::size() const -> std::int64_t
```

## What It Does

Returns the number of elements (rows) in the thread — the one dimension a `Thread`
has, regardless of payload kind. This is the same count [`shape()`](shape.md) reports
as its first element and [`ncases()`](ncases.md) returns under a different name.

## Parameters

`None`

## Returns

`std::int64_t`
    The element count. `0` for an empty (but still defined) thread.

## Examples

Example 1. A thread's size is independent of payload kind.

```cpp
using namespace pluss::table;

auto numbers = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto words = Thread(pluss::make_note({"a", "b"}));
std::cout << numbers.size() << "\n";
std::cout << words.size();
```

### Output

```text
3
2
```

Example 2. `size()` counts every element, including `NULL` ones — it's not a
non-null count.

```cpp
auto with_nulls = Thread(pluss::make_series({1.0, 2.0, 3.0}),
    pluss::make_series(std::vector<bool>{true, false, true}));
std::cout << with_nulls.size();
```

### Output

```text
3
```

## Notes

- Counts every element, valid or `NULL` — see [`count()`](count.md) for a
  non-null-only count.
- Calling `size()` on an undefined thread throws `"Thread: undefined handle"` — see
  [`defined()`](defined.md).

## See Also

- [`Thread::ncases`](ncases.md): the same count, under DataFrame-consistent naming.
- [`Thread::shape`](shape.md): `{size(), 1}` — the thread's shape as a pair.
- [`Thread::count`](count.md): the non-null element count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shuffle.html">
    <span class="label">Previous</span>
    <span class="title">shuffle</span>
  </a>
  <a class="next-link" href="skew.html">
    <span class="label">Next</span>
    <span class="title">skew</span>
  </a>
</div>
