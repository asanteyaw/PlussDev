<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 405866cfe223 -->

# Axel::unique

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
bool Axel::unique() const noexcept
```

## What It Does

Reports whether this handle is the sole owner of its underlying
storage — `true` only when the handle is defined *and* no other `Axel`
handle currently shares the same reference-counted storage.

```{important} This is a copy-on-write reference-count check, not a value-equality check
`unique()` says nothing about whether the *content* of this `Axel` is
distinct from any other `Axel`'s content — two independently-built
`Axel`s holding identical registries are each still `unique()`. It only
answers "does exactly one handle currently point at this storage?"
```

## Parameters

`None`

## Returns

`bool`
    `true` if `defined()` and the storage's reference count is exactly
    `1`; `false` otherwise (including for an undefined handle).

## Examples

Example 1. A freshly-built `Axel` with no other handle sharing its
storage is unique; copying it drops that to `false` on both handles
until the copy goes out of scope.

```cpp
auto axel = pluss::bundle::Axel({
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1)})}}),
});
std::cout << "before copy: " << std::boolalpha << axel.unique() << "\n";
auto copy = axel;
std::cout << "after copy: " << axel.unique() << " " << copy.unique();
```

### Output

```text
before copy: true
after copy: false false
```

## Notes

- An undefined handle (`Axel()`) is never `unique()` — it returns
  `false`, not a throw, since `unique()` (like `defined()`) is
  `noexcept`.
- `Axel`'s registries and rows are immutable from the outside; nothing
  in the public API lets a caller mutate through one handle and have it
  observed via another — `unique()` exists to inspect this
  reference-counting detail, not to gate a mutation decision the way it
  might in a hand-rolled copy-on-write container.

## See Also

- [`Axel::defined`](defined.md): whether the handle is bound to storage
  at all — `unique()` implies `defined()`, not the reverse.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
</div>
