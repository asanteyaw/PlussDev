<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: de16bcebc325 -->

# Stream::from_thread

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
static Stream Stream::from_thread(const pluss::table::Thread& thread, std::optional<pluss::table::Thread> cases = std::nullopt) -> Stream
```

## What It Does

A static factory that builds a `Stream` from a `pluss::table::Thread` — the inverse of
[`to_thread()`](to_thread.md), and the way to bring a `Thread` produced elsewhere in the
table stack (e.g. a `DataFrame` column extraction) into `Stream`'s API. The optional
`cases` argument attaches a row-label axis in the same call, equivalent to constructing the
`Stream` and then calling [`with_label()`](with_label.md).

## Parameters

**thread** (`const pluss::table::Thread&`)
    The source thread. Its payload kind becomes the resulting stream's kind.

**cases** (`std::optional<pluss::table::Thread>`, defaults to `std::nullopt`)
    An optional row-label axis to attach — see [`has_label()`](has_label.md)/
    [`lbl()`](lbl.md).

## Returns

`Stream`
    A new stream wrapping `thread`'s payload, with `cases` attached as its label axis if
    supplied.

## Examples

Example 1. Build a stream from a plain `Thread`.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto stream = pluss::kindle::Stream::from_thread(thread);
std::cout << stream;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 10                                                                                                       
1            | 20                                                                                                       
2            | 30                                                                                                       
========================================================================================================================
```

Example 2. Attach a row-label axis in the same call via `cases`.

```cpp
using pluss::table::Thread;

auto thread = Thread(pluss::make_series({10.0, 20.0}));
auto cases = Thread(pluss::make_note({"a", "b"}));

auto stream = pluss::kindle::Stream::from_thread(thread, cases);
std::cout << "has_label: " << stream.has_label();
```

### Output

```text
has_label: 1
```

## Notes

- `cases`, when supplied, must match `thread`'s length — the same requirement documented
  for [`with_label()`](with_label.md).
- A static method; call it as `Stream::from_thread(...)`, not on an existing instance.
- Round-trips with [`to_thread()`](to_thread.md): `Stream::from_thread(stream.to_thread())`
  reconstructs an equivalent (unlabeled) stream.

## See Also

- [`Stream::to_thread`](to_thread.md): the inverse conversion, from a `Stream` to a
  `Thread`.
- [`Stream::with_label`](with_label.md): attach a label axis after construction, instead of
  in the same call as this method's `cases` argument.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="has_label.html">
    <span class="label">Next</span>
    <span class="title">has_label</span>
  </a>
</div>
