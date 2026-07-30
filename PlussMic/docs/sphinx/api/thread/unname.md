<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 35c91b2aa485 -->

# Thread::unname

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::unname() const -> Thread
```

## What It Does

Removes an attached name from the thread, returning a new thread with no name — the
inverse of [`name(value)`](name.md)'s setter overload. The underlying values, validity
mask, and row labels are all carried over unchanged; only the name is cleared.

## Parameters

`None`

## Returns

`Thread`
    A new thread with no name attached, everything else unchanged. Non-mutating — the
    source thread is never modified, even if it had no name to begin with.

## Examples

Example 1. Attach a name, then remove it.

```cpp
using namespace pluss::table;

auto balance = Thread(pluss::make_series({100.0, 250.5, 75.25}));
auto named_balance = balance.name("balance");

std::cout << named_balance.named();

auto unnamed_again = named_balance.unname();

std::cout << unnamed_again.named();
```

### Output

```text
true
```

```text
false
```

`named_balance` itself is left unmodified — `unname()` is non-mutating, so
`unnamed_again` is a distinct copy with the name cleared.

Example 2. Calling `unname()` on a thread that already has no name is a no-op; it still
returns a valid (still-unnamed) copy rather than throwing.

```cpp
auto still_unnamed = balance.unname();

std::cout << still_unnamed.named();
```

### Output

```text
false
```

## Notes

- Non-mutating — always returns a new `Thread`, even when there was no name to remove.
- After `unname()`, [`named()`](named.md) reports `false` and [`name()`](name.md)'s getter
  throws again.
- Values, validity mask, and row labels are unaffected — only the name is cleared.

## See Also

- [`Thread::name`](name.md): attach a name (the setter overload this method reverses).
- [`Thread::named`](named.md): check whether a name is currently attached.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ts.html">
    <span class="label">Previous</span>
    <span class="title">ts</span>
  </a>
  <a class="next-link" href="upper.html">
    <span class="label">Next</span>
    <span class="title">upper</span>
  </a>
</div>
