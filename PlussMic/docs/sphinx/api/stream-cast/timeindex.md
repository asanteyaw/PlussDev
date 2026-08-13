<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c89d6b313a20 -->

# StreamCastAccessor::timeindex

[Back to StreamCastAccessor API Reference](../../stream_cast_accessor_api_reference)

## Signature

```cpp
Stream StreamCastAccessor::timeindex() const
```

## What It Does

Casts the bound stream's values to `TimeIndex`-kind and returns a new stream.
Behaves the same as [`timeline()`](timeline.md), including throwing (rather
than nulling) on unparseable text.

## Parameters

`None`

## Returns

`Stream`
    A new stream cast to `TimeIndex`-kind.

## Examples

Example 1. Cast a `Note` stream of date-looking text to `TimeIndex`.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"2024-01-01", "2024-01-02"}));
auto out = stream.cast().timeindex();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=TimeIndex  dtype=TimeIndex  records=2
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 2024-01-01T00:00:00.000000000+00:00                                                                      
1            | 2024-01-02T00:00:00.000000000+00:00                                                                      
========================================================================================================================
```

Example 2 (edge case). Text that doesn't parse as a datetime throws.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra"}));
try {
  auto out = stream.cast().timeindex();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
moment: failed to parse datetime
```

## Notes

- Requires a defined stream; throws `"Stream: undefined handle"` on a
  default-constructed `Stream()`.

## See Also

- [`StreamCastAccessor::timeline`](timeline.md): the related cast target,
  with the same parse-failure throwing behavior.
- [`StreamCastAccessor::series`](series.md): the numeric cast target, with a
  different (silent-null) failure mode for unparseable input.
- [`Stream::cast`](../stream/cast.md): the direct-target overload of casting,
  covering the same conversions.
- [`CatalogCastAccessor::timeindex`](../catalog-cast/timeindex.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="series.html">
    <span class="label">Previous</span>
    <span class="title">series</span>
  </a>
  <a class="next-link" href="timeline.html">
    <span class="label">Next</span>
    <span class="title">timeline</span>
  </a>
</div>
