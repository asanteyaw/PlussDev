<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ad55d89637ea -->

# Axel::has

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
bool Axel::has(std::string_view field) const
```

## What It Does

Checks whether `field` exists in **any** registry frame across the
depth stack.

```{important} "any", not "every"
`has()` returns `true` as soon as *one* frame contains `field` — it
does not mean every frame does. `Axel` does not require every frame to
share an identical schema, so a `true` result here doesn't guarantee
[`fields({field})`](fields.md) will succeed on the whole `Axel` (that
call requires the field to exist in every frame it's applied to and
throws otherwise).
```

## Parameters

**field** (`std::string_view`)
    The field name to look for.

## Returns

`bool`
    `true` if at least one frame has a field named `field`; `false` if
    no frame does.

## Examples

Example 1. Two frames with disjoint schemas — `has()` is `true` for a
field that only exists in one of them.

```cpp
auto r0 = pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1)})}});
auto r1 = pluss::bundle::Registry({{"city", pluss::bundle::Slot({pluss::make_note("Accra")})}});
auto axel = pluss::bundle::Axel({r0, r1});

std::cout << std::boolalpha << axel.has("ids") << " " << axel.has("city") << " " << axel.has("zzz");
```

### Output

```text
true true false
```

`"ids"` only exists in frame `0` and `"city"` only in frame `1`, but
both report `true` — `has()` only needs a single matching frame.

## Notes

- See the important callout above — don't treat a `true` result as
  "every frame has this field."
- Never throws, regardless of the answer — same contract as
  `Registry::has()`.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"`.

## See Also

- [`Axel::fields`](fields.md): the operation that actually requires a
  field to exist in every frame it touches.
- [`Registry::has`](../registry/has.md): the single-frame equivalent
  this method loops over internally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_kindle.html">
    <span class="label">Previous</span>
    <span class="title">from_kindle</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>
