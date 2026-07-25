<!-- pluss-hand-authored -->

# Thread::defined

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::defined() const noexcept -> bool
```

## What It Does

Reports whether the `Thread` object currently owns a usable backing payload at all —
an object-level integrity check, not a data-level one. A default-constructed
`Thread()` is undefined: it has no `Series`, `Note`, `Timeline`, or `TimeIndex`
payload behind it, and calling almost any other method on it throws. This is a
different question from "does this thread have missing values" — a fully defined
thread can still hold `NULL` cells; see [`isnull()`](isnull.md) for that question.

```{important} Almost every other method requires defined() == true
Calling `size()`, `kind()`, `dtype()`, or any transform/reduction on an undefined
thread throws `"Thread: undefined handle"`. `defined()` itself is one of the very
few methods safe to call unconditionally — it never throws, regardless of whether
the thread is defined.
```

## Parameters

`None`

## Returns

`bool`
    `true` when the thread owns a backing payload and can participate in ordinary
    operations; `false` for a default-constructed or moved-from `Thread`.

## Examples

Example 1. A thread built from real data is defined.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto out = scores.defined();
std::cout << std::boolalpha << out;
```

### Output

```text
true
```

Example 2. A default-constructed thread is not defined.

```cpp
auto empty_handle = Thread();
auto out2 = empty_handle.defined();
std::cout << std::boolalpha << out2;
```

### Output

```text
false
```

Example 3. Calling another method on an undefined thread throws.

```cpp
try {
  auto bad = empty_handle.size();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread: undefined handle
```

## Notes

- `defined()` never throws, even on an undefined thread — it's the safe way to check
  before calling anything else.
- Object-level validity only: says nothing about whether individual cells are
  `NULL`. Use [`isnull()`](isnull.md) / [`notnull()`](notnull.md) for that.
- A moved-from `Thread` (via `std::move`) is also left undefined.

## See Also

- [`Thread::isnull`](isnull.md) / [`Thread::notnull`](notnull.md): per-cell validity,
  as opposed to this method's whole-object validity.
- [`Thread::shares_payload_with`](shares_payload_with.md): a related object-identity
  check, comparing two defined threads' underlying storage.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="day.html">
    <span class="label">Previous</span>
    <span class="title">day</span>
  </a>
  <a class="next-link" href="describe.html">
    <span class="label">Next</span>
    <span class="title">describe</span>
  </a>
</div>
