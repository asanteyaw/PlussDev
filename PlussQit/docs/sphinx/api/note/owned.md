<!-- pluss-hand-authored -->
# Note::owned

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
TextRef Note::owned() const
```

## Retain The Only Cell

`owned()` returns the Note's sole cell as an owning `TextRef`, without requiring
the caller to spell position zero. It is intended for APIs that accept any
one-element Note geometry and need to return or store its text past the Note's
lifetime. The receiver must have exactly one element, but it need not have rank
zero: shapes `[1]`, `[1, 1]`, and `[1, 1, 1]` are valid too.

That contract deliberately follows `numel()`, not `is_scalar()`. Use
`is_scalar()` first when rank-zero input is itself a requirement. Empty text is
a valid one-element value and produces a defined, empty `TextRef`; a Note with
zero elements is rejected.

The operation is constant-time and does not copy bytes. The returned handle
co-owns the complete packed arena. Its `view()` is borrowed from the handle,
while `str()` materializes an independent `std::string`. `owned()` rejects an
undefined Note and every Note whose element count is not exactly one.

## Example 1: Extract From A Temporary Scalar

```cpp
const pluss::TextRef value = pluss::note::make("solo").owned();
std::cout << value << '\n';
```

### Output

```text
solo
```

The temporary Note may disappear at the semicolon; `value` keeps its arena.

## Example 2: Singleton Geometry Is Accepted

```cpp
const auto one_matrix = pluss::note::make(
    std::vector<std::string>{"one"},
    std::array<std::int64_t, 2>{1, 1});
std::cout << "matrix=" << one_matrix.owned() << '\n';
```

### Output

```text
matrix=one
```

The result contains only text, not the source `[1, 1]` geometry.

## Example 3: Empty Text Is A Real Cell

```cpp
const auto value = pluss::note::make("").owned();
std::cout << std::boolalpha
          << "empty defined=" << value.defined()
          << " empty=" << value.empty()
          << " bytes=" << value.size_bytes() << '\n';
```

### Output

```text
empty defined=true empty=true bytes=0
```

## Example 4: Zero And Multiple Elements Reject

```cpp
const auto zero = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 1>{0});
const auto many = pluss::note::make({"a", "b"});
auto show = [](std::string_view label, const pluss::Note& note) {
  try { (void)note.owned(); }
  catch (const pluss::Error&) { std::cout << label << ": rejected\n"; }
};
show("zero", zero);
show("many", many);
show("undefined", pluss::Note::undefined());
```

### Output

```text
zero: rejected
many: rejected
undefined: rejected
```

## See Also

- [`cell`](cell.md): owning access when a flat position is needed.
- [`cell_at`](cell_at.md): owning access by a full coordinate.
- [`is_scalar`](is_scalar.md): require rank zero rather than one element.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="operators.html"><span class="label">Previous</span><span class="title">operators</span></a>
  <a class="next-link" href="pack.html"><span class="label">Next</span><span class="title">pack</span></a>
</div>
