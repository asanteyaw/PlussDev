<!-- pluss-hand-authored -->
# Note::is_bool

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
bool Note::is_bool() const noexcept
```

## Test The Storage Type, Not The Cell Values

`is_bool()` is true only when the receiver stores native Bool lanes. It does
not ask whether text happens to contain `"true"` or `"false"`, and it does not
reduce a Bool Note to one truth value. Use [`any`](any.md) or [`all`](all.md)
for reductions; use `bools()` when host code needs the lanes themselves.

Predicates and comparisons return Bool Notes, so `is_bool()` is also a useful
contract check at the boundary between mask construction and mask consumption.
Rank and shape are irrelevant: a scalar, matrix, and empty array can all be
Bool. The query is constant-time, allocation-free, and `noexcept`; undefined
returns `false`.

## Example 1: Distinguish Bool From Text And Absence

```cpp
const auto flags = pluss::note::make(std::vector<bool>{true, false});
const auto text = pluss::note::make("alpha");
const auto absent = pluss::Note::undefined();
std::cout << std::boolalpha
          << "flags=" << flags.is_bool()
          << " text=" << text.is_bool()
          << " absent=" << absent.is_bool() << '\n';
```

### Output

```text
flags=true text=false absent=false
```

## Example 2: Predicate Results Are Bool Notes

```cpp
const auto text = pluss::note::make("alpha");
std::cout << std::boolalpha
          << "predicate=" << text.contains("a").is_bool() << '\n';
```

### Output

```text
predicate=true
```

## Example 3: Higher Rank Remains Bool

```cpp
const auto grid = pluss::note::make(
    std::vector<bool>{true, false, false, true},
    std::array<std::int64_t, 2>{2, 2});
std::cout << std::boolalpha << "bool=" << grid.is_bool()
          << " shape=[" << grid.size(0) << ", " << grid.size(1) << "]\n";
```

### Output

```text
bool=true shape=[2, 2]
```

## See Also

- [`Note::dtype`](dtype.md): obtain the complete dtype enum.
- [`Note::any`](any.md) and [`Note::all`](all.md): reduce Bool lanes to a
  scalar Bool Note.
- `Note::bools()`: borrow the contiguous `std::uint8_t` lanes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_blank.html"><span class="label">Previous</span><span class="title">is_blank</span></a>
  <a class="next-link" href="is_bytes.html"><span class="label">Next</span><span class="title">is_bytes</span></a>
</div>
