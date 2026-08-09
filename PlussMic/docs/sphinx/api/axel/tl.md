<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 10e018ed96d2 -->

# Axel::tl

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
AxelTimelineAccessor Axel::tl() const
```

## What It Does

Returns an [`AxelTimelineAccessor`](../../axel_timeline_accessor_api_reference)
bound to this `Axel`, for applying `Timeline`-kind operations (calendar
fields, formatting, timezone handling) across every field of every
registry frame in the depth stack.

```{important} Scope with `fields()` first
Like [`cast()`](cast.md) and [`nt()`](nt.md), `tl()` itself takes no
field names — it applies to every field of every frame. Chain
`axel.fields({"..."}).tl()` to scope the effect to one field.
```

## Parameters

`None`

## Returns

`AxelTimelineAccessor`
    An accessor exposing the full `Timeline` method set (calendar
    fields like `year()`/`month()`/`weekday()`, formatting like
    `fmt()`/`iso()`, and conversion like `tz()`/`shift()`/`index()`) —
    see the
    [`AxelTimelineAccessor` reference](../../axel_timeline_accessor_api_reference).

## Examples

Example 1. Extract the year from the `"ts"` field of every frame.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")})}});
auto r1 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2023-07-04T00:00:00Z")})}});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.fields({"ts"}).tl().year();
std::cout << std::get<pluss::Series>(out.registry(0).fields("ts").at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.registry(1).fields("ts").at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
2024 2023
```

## Notes

- Every field the accessor touches must be `Timeline`-kind in every
  frame it's applied to — a non-`Timeline` field throws whatever the
  underlying `Registry::tl()` call throws for that frame.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"`.

## See Also

- [`AxelTimelineAccessor` API Reference](../../axel_timeline_accessor_api_reference):
  every method available on the returned accessor.
- [`Axel::fields`](fields.md): narrow to a specific field before
  applying timeline operations.
- [`Axel::nt`](nt.md): the equivalent entry point for `Note`-kind
  fields.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="to_json.html">
    <span class="label">Next</span>
    <span class="title">to_json</span>
  </a>
</div>
