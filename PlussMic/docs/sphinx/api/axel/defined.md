<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3685d9f2e68b -->

# Axel::defined

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
bool Axel::defined() const noexcept
```

## What It Does

Reports whether the `Axel` handle is bound to actual storage. A
default-constructed `Axel` (`Axel()`) is undefined; every other
constructor — including one built from an empty vector of registries —
produces a defined handle. Almost every other `Axel` method requires
`defined()` to be `true` and throws `"Axel: undefined handle"`
otherwise.

## Parameters

`None`

## Returns

`bool`
    `true` if the handle is bound to storage, `false` for a
    default-constructed handle.

## Examples

Example 1. A default-constructed `Axel` is undefined; one built from a
registry vector — even an empty one — is defined.

```cpp
auto axel = pluss::bundle::Axel({
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})}}),
});
pluss::bundle::Axel undef;
auto empty_axel = pluss::bundle::Axel(std::vector<pluss::bundle::Registry>{});
std::cout << std::boolalpha << axel.defined() << " " << undef.defined() << " " << empty_axel.defined();
```

### Output

```text
true false true
```

An `Axel` built from zero registries is still `defined()` — `empty()`
is the check for zero depth, not `defined()`. See [`empty()`](empty.md).

## Notes

- `defined()` is `noexcept` and never throws, unlike almost every other
  `Axel` method — it's the one safe check to run on a handle you're not
  sure about.
- Copying or moving an `Axel` copies/moves the underlying handle
  reference (via an intrusive pointer), so a copy of a defined `Axel` is
  also defined.

## See Also

- [`Axel::unique`](unique.md): whether this handle is the sole owner of
  its storage — a separate, stronger question than `defined()`.
- [`Axel::empty`](empty.md): whether a *defined* handle has zero depth.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="concat_depth.html">
    <span class="label">Previous</span>
    <span class="title">concat_depth</span>
  </a>
  <a class="next-link" href="depth.html">
    <span class="label">Next</span>
    <span class="title">depth</span>
  </a>
</div>
