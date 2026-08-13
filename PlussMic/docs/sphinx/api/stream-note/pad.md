<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4092db19dad3 -->

# StreamNoteAccessor::pad

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signature

```cpp
Stream StreamNoteAccessor::pad(std::int64_t width, pluss::table::TextAlign align = pluss::table::TextAlign::Left, char fill = ' ') const
```

## What It Does

Pads the bound stream's text out to `width` characters using `fill`, aligned
per `align`, returning a new stream. Text already at or beyond `width` is left
unchanged.

## Parameters

**width** (`std::int64_t`)
    The target minimum character width.

**align** (`pluss::table::TextAlign`, default `Left`)
    Where the original text lands within the padded width: `Left` (padding
    added on the right), `Right` (padding on the left), or `Center`.

**fill** (`char`, default `' '`)
    The character used to pad.

## Returns

`Stream`
    A new stream with its text padded to at least `width` characters.

## Examples

Example 1. Right-align a numeric-looking string, zero-padded to width 3.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"5"}));
auto out = stream.nt().pad(3, pluss::table::TextAlign::Right, '0');
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Note  dtype=Note  records=1
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 005                                                                                                      
========================================================================================================================
```

## Notes

- Text already at or beyond `width` is returned unchanged, not truncated.
- The inverse operation of [`strip()`](strip.md) — adding characters instead
  of removing them.

## See Also

- [`StreamNoteAccessor::strip`](strip.md): removes characters instead of
  adding them.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::pad`](../catalog-note/pad.md): the whole-catalog
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normalize.html">
    <span class="label">Previous</span>
    <span class="title">normalize</span>
  </a>
  <a class="next-link" href="partition.html">
    <span class="label">Next</span>
    <span class="title">partition</span>
  </a>
</div>
