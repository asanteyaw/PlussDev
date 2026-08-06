<!-- pluss-hand-authored -->

# Axel::from_kindle

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
static Axel Axel::from_kindle(const pluss::kindle::Archive& archive)
```

## What It Does

Static factory that reconstructs an `Axel` from a `pluss::kindle::Archive`
— the reverse of [`to_kindle()`](to_kindle.md). Each catalog in the
archive becomes a registry frame (via
[`Registry::from_kindle()`](../registry/from_kindle.md)), and the
archive's label stream, if present, is converted back into the `Axel`'s
label `Slot`.

## Parameters

**archive** (`const pluss::kindle::Archive&`)
    The archive to convert back. Must be defined.

## Returns

`Axel`
    A new axel with `archive.depth()` frames, reconstructed in the same
    order, with labels restored if the archive carried a label stream.

## Examples

Example 1. Round-trip an `Axel` through `to_kindle()` and back.

```cpp
auto axel = pluss::bundle::Axel({
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1)})}}),
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(2)})}}),
});
auto arc = axel.to_kindle();
auto back = pluss::bundle::Axel::from_kindle(arc);
std::cout << "depth=" << back.depth() << " id0="
          << std::get<pluss::Series>(back.registry(0).fields("ids").at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
depth=2 id0=1
```

Example 2. An undefined archive throws.

```cpp
try {
  pluss::kindle::Archive undef;
  pluss::bundle::Axel::from_kindle(undef);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Axel::from_kindle: archive undefined
```

## Notes

- `archive` must be `defined()`, or the call throws
  `"Axel::from_kindle: archive undefined"`.
- Each catalog's conversion can throw whatever
  `Registry::from_kindle()` throws for it.
- This is a `static` member — call it as `Axel::from_kindle(archive)`,
  not on an existing instance.

## See Also

- [`Axel::to_kindle`](to_kindle.md): the forward conversion this method
  reverses.
- [`Registry::from_kindle`](../registry/from_kindle.md): the per-frame
  reconstruction this method calls for each catalog.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
