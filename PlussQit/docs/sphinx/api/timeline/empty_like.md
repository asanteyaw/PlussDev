<!-- pluss-hand-authored -->

# Timeline::empty_like

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::empty_like() const
```

## Create A Zero-Record Timeline With Compatible Geometry

`empty_like()` creates an empty `Timeline` whose trailing dimensions and
timezone come from the receiver. It is the structural identity for axis-0
operations such as [`concat()`](concat.md): there are no records yet, but the
shape of each future record is already known.

The word “empty” is literal. This method does not allocate uninitialized
timestamps with the receiver's full shape. Temporal values cannot safely expose
uninitialized bit patterns as instants. Instead, axis 0 becomes zero:

| Receiver shape | Result shape |
| --- | --- |
| scalar `[]` | `[0]` |
| vector `[n]` | `[0]` |
| matrix `[n, d]` | `[0, d]` |
| array `[n, d, e]` | `[0, d, e]` |

A scalar therefore becomes an empty one-dimensional sequence. This promotion
matches scalar `concat()` and `repeat()` behavior: once an operation concerns
record count, the scalar is treated as one record.

## Values, Timezone, And Ownership

No receiver values are read or retained. The result has `numel()==0`, owns its
small empty storage object, and keeps the receiver's timezone name. Calling the
method on a strided timeline is constant in the receiver's element count
because there is no payload to copy.

Example 1. A scalar has no trailing geometry, so its empty counterpart is a
length-zero vector.

```cpp
const auto scalar = pluss::timeline::date("2024-01-01");
std::cout << pluss::timeline::print(scalar.empty_like()) << '\n';
```

### Output

```text
Timeline(shape=[0], tz=UTC)[  ]
```

Example 2. A `[2, 1, 2]` receiver contributes trailing shape `[1, 2]`; only
the record axis is emptied.

```cpp
const auto cube = pluss::timeline::date({
    {{"2024-01-01", "2024-01-02"}},
    {{"2024-01-03", "2024-01-04"}},
});
std::cout << pluss::timeline::print(cube.empty_like()) << '\n';
```

### Output

```text
Timeline(shape=[0, 1, 2], tz=UTC)
```

Example 3. The timezone is structural metadata and survives even though no
instants remain.

```cpp
const auto local_empty =
    scalar.tz_convert("America/Edmonton").empty_like();
std::cout << "shape=[" << local_empty.sizes()[0]
          << "] numel=" << local_empty.numel()
          << " tz=" << local_empty.tz() << '\n';
```

### Output

```text
shape=[0] numel=0 tz=America/Edmonton
```

## Practical Use

Use `empty_like()` when an algorithm accumulates zero or more compatible
temporal blocks and needs a correctly shaped starting value. Do not use it when
you need the receiver's full shape filled with a known instant; that is
[`fill_like()`](fill_like.md). Do not use it as a placeholder whose elements
will later be assigned, because it contains no elements to assign.

The operation has no data-dependent failure mode once the receiver is a valid
`Timeline`. Rank remains within Timeline's supported scalar-through-3-D range,
and changing the leading extent to zero cannot overflow.

## See Also

- [`Timeline::concat`](concat.md): append compatible axis-0 records.
- [`Timeline::fill_like`](fill_like.md): retain the complete shape and fill it.
- [`Timeline::repeat`](repeat.md): replicate the receiver's axis-0 block.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dim.html"><span class="label">Previous</span><span class="title">dim</span></a>
  <a class="next-link" href="epoch_days.html"><span class="label">Next</span><span class="title">epoch_days</span></a>
</div>
