<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 61da929d72ac -->

# StreamCastAccessor::timeline

[Back to StreamCastAccessor API Reference](../../stream_cast_accessor_api_reference)

## Signature

```cpp
Stream StreamCastAccessor::timeline() const
```

## What It Does

Casts the bound stream's values to `Timeline`-kind and returns a new stream.

```{important} Unparseable text throws — unlike casting toward Series
Casting toward `Timeline` behaves differently from [`series()`](series.md)'s
silent-null failure mode: text that doesn't parse as a datetime throws
`"moment: failed to parse datetime"` immediately, rather than producing a
null value.
```

## Parameters

`None`

## Returns

`Stream`
    A new stream cast to `Timeline`-kind.

## Examples

Example 1. Cast a `Note` stream of date-looking text to `Timeline`.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"2024-01-01", "2024-01-02"}));
auto out = stream.cast().timeline();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Timeline  dtype=Timeline  records=2
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 2024-01-01T00:00:00.000000000+00:00                                                                      
1            | 2024-01-02T00:00:00.000000000+00:00                                                                      
========================================================================================================================
```

Example 2 (edge case). Text that doesn't parse as a datetime throws — see the
important callout above.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra"}));
try {
  auto out = stream.cast().timeline();
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

- Throws on parse failure — see the important callout above, and compare with
  [`series()`](series.md)'s silent-null behavior for the same kind of input.

## See Also

- [`StreamCastAccessor::timeindex`](timeindex.md): the related cast target,
  with the same parse-failure throwing behavior.
- [`StreamCastAccessor::series`](series.md): the numeric cast target, with a
  different (silent-null) failure mode for unparseable input.
- [`Stream::cast`](../stream/cast.md): the direct-target overload of casting,
  covering the same conversions.
- [`CatalogCastAccessor::timeline`](../catalog-cast/timeline.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="timeindex.html">
    <span class="label">Previous</span>
    <span class="title">timeindex</span>
  </a>
</div>
