<!-- pluss-hand-authored -->

# Nexus::unique

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::unique() const noexcept -> bool
```

## What It Does

Reports whether this `Nexus` handle is the **sole** owner of its underlying
workspace — a copy-on-write signal, not a data question. `Nexus` handles are cheap to
copy (copying just bumps a reference count); the actual table data is only cloned the
moment a mutating method (any trailing-underscore method, like
`rename_()` or `inject_()`) is called on a handle that
isn't the sole owner. `unique()` lets you observe that bookkeeping directly.

## Parameters

`None`

## Returns

`bool`
    `true` if no other `Nexus` handle currently shares this workspace; `false` if at
    least one other handle does.

## Examples

Example 1. A freshly constructed `Nexus` is its own sole owner.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", pluss::make_series({1.0, 2.0, 3.0})},
    {"name",        pluss::make_note({"Ada", "Bola", "Chen"})},
});

auto orders = DataFrame({
    {"order_id",    pluss::make_series({100.0, 101.0, 102.0})},
    {"customer_id", pluss::make_series({1.0, 2.0, 2.0})},
    {"amount",      pluss::make_series({50.0, 75.0, 20.0})},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
});

std::cout << std::boolalpha << biz.unique();
```

### Output

```text
true
```

Example 2. Copying the handle breaks uniqueness on **both** sides.

```cpp
auto shared = biz;
std::cout << std::boolalpha << biz.unique() << " " << shared.unique();
```

### Output

```text
false false
```

`shared` is a new handle pointing at the same underlying workspace as `biz` — no
table data was copied, just a reference. Both handles now report `unique() ==
false`, since neither is the sole owner anymore.

Example 3. A mutating call on one handle clones the workspace, restoring uniqueness
on both sides independently.

```cpp
shared.rename_("orders", "sales");

std::cout << std::boolalpha << biz.unique() << " " << shared.unique() << "\n";
std::cout << biz.has("orders") << " " << shared.has("orders") << " " << shared.has("sales");
```

### Output

```text
true true
true false true
```

`rename_()` is a mutating (trailing-underscore) method. Since `shared` wasn't the
sole owner when it was called, the workspace was cloned first — `shared` now points
at its own independent copy, renamed as requested, while `biz` still has its
original, untouched `"orders"` table. Both handles are sole owners again after the
clone.

## Notes

- Copying a `Nexus` handle (`auto shared = biz;`) never copies table data up front —
  it shares the same workspace and increments a reference count, which is why
  `unique()` immediately becomes `false` on both sides.
- The clone only happens lazily, inside a mutating (trailing-underscore) method, and
  only if `unique()` was already `false` at that moment — mutating a handle that's
  already the sole owner never clones.
- Non-mutating methods (no trailing underscore, like [`rename()`](rename.md) or
  [`select()`](select.md)) always return a brand-new `Nexus` and never touch
  `unique()` on the original handle at all.

## See Also

- [`Nexus::defined`](defined.md): whether the handle owns a workspace at all, a
  different existence question from sole ownership.
- `rename_()` / `inject_()`: mutating methods whose clone-on-write behavior this
  page demonstrates.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="validate_keys.html">
    <span class="label">Next</span>
    <span class="title">validate_keys</span>
  </a>
</div>
