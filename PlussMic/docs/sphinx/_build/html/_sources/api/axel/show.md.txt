<!-- pluss-hand-authored -->

# Axel::show

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
void Axel::show(std::ostream& os) const
```

## What It Does

Writes the exact same summary string [`to_string()`](to_string.md)
produces to `os`. `operator<<` for `Axel` calls this method internally,
so `os << axel` and `axel.show(os)` are equivalent.

## Parameters

**os** (`std::ostream&`)
    The stream to write the summary to.

## Returns

`void`

## Examples

Example 1. `show()`'s output matches `to_string()`'s return value
exactly.

```cpp
auto axel = pluss::bundle::Axel({
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1)})}}),
});
std::ostringstream oss;
axel.show(oss);
std::cout << std::boolalpha << (oss.str() == axel.to_string());
```

### Output

```text
true
```

## Notes

- `show()` is `to_string()`'s content routed to a stream — see
  [`to_string()`](to_string.md) for the format, including its
  depth-`6` truncation behavior and the important asymmetry with
  [`to_json()`](to_json.md).
- `std::ostream& operator<<(std::ostream&, const Axel&)` calls `show()`
  internally, so `std::cout << axel` is equivalent to
  `axel.show(std::cout)`.

## See Also

- [`Axel::to_string`](to_string.md): the string this method streams.
- [`Axel::to_json`](to_json.md): the alternative, unabridged dump.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rename.html">
    <span class="label">Previous</span>
    <span class="title">rename</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
