<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 405866cfe223 -->

# Atlas::unique

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::unique() const noexcept -> bool
```

## What It Does

Reports whether **this `Atlas` handle is the sole owner** of its underlying frame stack —
a reference-count check on the internal `intrusive_ptr`, not a check on the contained
frames' values.

```{important} unique() is about ownership, not distinct values or frames
`unique()` has nothing to do with whether the stacked frames are distinct from one
another — an `Atlas` built from the same `DataFrame` repeated three times still reports
`unique(): true` if no other `Atlas` handle shares its payload. Conversely, an `Atlas`
whose frames are all different reports `unique(): false` the moment a copy of the handle
exists anywhere.
```

`Atlas`'s copy constructor is a cheap handle copy (shared ownership, like
`std::shared_ptr`), not a deep copy of every contained `DataFrame` — `unique()` tells you
whether such a sharing copy currently exists. It goes back to `true` once every other
sharing handle goes out of scope.

## Parameters

`None`

## Returns

`bool`
    `true` if no other `Atlas` handle currently shares this atlas's payload; `false` if
    at least one copy exists elsewhere.

## Examples

Example 1. A freshly constructed atlas is its own sole owner.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"sales", Thread(pluss::make_series({100.0, 120.0}))}});
auto q2 = DataFrame({{"sales", Thread(pluss::make_series({110.0, 90.0}))}});
auto atlas = Atlas({q1, q2});

std::cout << atlas.unique();
```

### Output

```text
1
```

Example 2. Copying the atlas drops `unique()` to `false` on *both* handles, for as long
as both are alive.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"sales", Thread(pluss::make_series({100.0, 120.0}))}});
auto atlas1 = Atlas({q1});
std::cout << "atlas1.unique() before copy: " << atlas1.unique() << "\n";

auto atlas2 = atlas1;
std::cout << "atlas1.unique() with atlas2 alive: " << atlas1.unique() << "\n";
std::cout << "atlas2.unique(): " << atlas2.unique() << "\n";
```

### Output

```text
atlas1.unique() before copy: 1
atlas1.unique() with atlas2 alive: 0
atlas2.unique(): 0
```

Example 3. Once the sharing copy goes out of scope, `unique()` returns to `true`.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"sales", Thread(pluss::make_series({100.0, 120.0}))}});
auto atlas1 = Atlas({q1});
{
  auto atlas2 = atlas1;
  std::cout << "while atlas2 alive: " << atlas1.unique() << "\n";
}
std::cout << "after atlas2 destroyed: " << atlas1.unique();
```

### Output

```text
while atlas2 alive: 0
after atlas2 destroyed: 1
```

## Notes

- Ownership-sharing check, not a value-distinctness check — see the important callout
  above.
- Moving an atlas (`std::move`) does **not** create a second owner — the moved-from
  handle is left empty, so the moved-to handle still reports `unique(): true`.
- Methods that derive a new atlas (e.g. [`head()`](head.md), [`features()`](features.md))
  allocate a fresh payload, so the result is its own sole owner, independent of the
  receiver's `unique()` status.
- `noexcept`; safe to call at any time, including on an undefined `Atlas`.

## See Also

- [`Atlas::defined`](defined.md): a different ownership-adjacent check — whether the
  handle has a payload at all, regardless of sharing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
