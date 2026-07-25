<!-- pluss-hand-authored -->

# Thread::is_valid

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::is_valid(std::int64_t row) const -> bool
```

## What It Does

Checks whether a single position in the thread is valid (not `NULL`), the way
[`isnull()`](isnull.md)/[`notnull()`](notnull.md) check every position at once and
return a `Bool` thread. `row` supports Python-style negative indexing (`-1` is the
last element), and a thread with no validity mask attached at all is treated as
fully valid everywhere.

## Parameters

**row** (`std::int64_t`)
    The position to check. Negative values count from the end (`-1` is the last
    row). Out-of-range values (after resolving negatives) throw `"Thread: index out
    of bounds"`.

## Returns

`bool`
    `true` if the element at `row` is valid; `false` if it's `NULL`.

## Examples

Example 1. Check specific positions, including a negative index.

```cpp
using namespace pluss::table;

auto readings = Thread(pluss::make_series({10.0, 20.0, 30.0}),
    pluss::make_series(std::vector<bool>{true, false, true}));

std::cout << std::boolalpha << readings.is_valid(0) << "\n";
std::cout << std::boolalpha << readings.is_valid(1) << "\n";
std::cout << std::boolalpha << readings.is_valid(-1);
```

### Output

```text
true
false
true
```

Row `1` was constructed invalid, so `is_valid(1)` is `false`. `is_valid(-1)` resolves
to the last row (index `2`), which is valid.

Example 2. A thread with no validity mask reports every position as valid.

```cpp
auto plain = Thread(pluss::make_series({1.0, 2.0}));
std::cout << std::boolalpha << plain.is_valid(0) << " " << plain.is_valid(1);
```

### Output

```text
true true
```

Example 3. An out-of-range row throws.

```cpp
try {
  auto bad = readings.is_valid(10);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread: index out of bounds
```

## Notes

- Negative `row` values count from the end, same convention as Python indexing.
- A thread with no validity mask attached reports `true` for every position — there
  is no separate "has no mask" state exposed here.
- Out-of-range `row` (after negative-index resolution) throws `"Thread: index out of
  bounds"`.

## See Also

- [`Thread::isnull`](isnull.md) / [`Thread::notnull`](notnull.md): the same check
  applied elementwise across the whole thread at once.
- [`Thread::defined`](defined.md): whole-object validity, as opposed to this
  method's single-element check.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="invalid.html">
    <span class="label">Previous</span>
    <span class="title">invalid</span>
  </a>
  <a class="next-link" href="isfinite.html">
    <span class="label">Next</span>
    <span class="title">isfinite</span>
  </a>
</div>
