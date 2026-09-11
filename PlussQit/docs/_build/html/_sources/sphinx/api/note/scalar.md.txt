<!-- pluss-hand-authored -->
# Note::scalar

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::string_view Note::scalar() const
```

## Borrow The Only Cell

`scalar()` returns the receiver's sole cell as a non-owning `std::string_view`.
Its contract is about **cardinality**, not rank: any defined `Note` with
`numel() == 1` is accepted. A true rank-zero scalar, a shape `[1]` vector, a
shape `[1, 1]` matrix, and a shape `[1, 1, 1]` tensor therefore all have a
scalar projection. This is deliberately broader than [`is_scalar()`](is_scalar.md),
which asks whether the Note has rank zero.

The returned view names the exact stored bytes. No string is allocated, no
terminating zero is appended, and no UTF-8 validation is performed. An empty
text cell is consequently a valid scalar result with `size() == 0`; it must not
be confused with a zero-element Note, which has no cell to return.

Because the view borrows the packed text arena, it remains valid only while a
`Note` or `TextRef` still owns that arena. Use [`owned()`](owned.md) for a
zero-copy handle that keeps storage alive, or construct `std::string(n.scalar())`
when an independent mutable copy is wanted. The lookup itself is constant-time
and allocation-free.

## Example 1: Read A Rank-Zero Scalar

```cpp
const auto n = pluss::note::make("alpha");
std::cout << "rank0=" << n.scalar() << '\n';
```

### Output

```text
rank0=alpha
```

## Example 2: Cardinality, Not Rank, Controls Acceptance

```cpp
const auto vector = pluss::note::make({"beta"});
const auto matrix = pluss::note::make(
    std::vector<std::string>{"gamma"},
    std::array<std::int64_t, 2>{1, 1});
const auto tensor = pluss::note::make(
    std::vector<std::string>{"delta"},
    std::array<std::int64_t, 3>{1, 1, 1});

std::cout << "rank1=" << vector.scalar() << '\n'
          << "rank2=" << matrix.scalar() << '\n'
          << "rank3=" << tensor.scalar() << '\n';
```

### Output

```text
rank1=beta
rank2=gamma
rank3=delta
```

This is useful after a selection that leaves one cell but intentionally keeps
one or more size-one axes.

## Example 3: Empty Text Is Still One Cell

```cpp
const auto text = pluss::note::make("").scalar();
std::cout << "empty text bytes=" << text.size() << '\n';
```

### Output

```text
empty text bytes=0
```

## Example 4: Preserve The Result Past The Source Lifetime

```cpp
std::string retained;
{
  const auto temporary = pluss::note::make({"keep", "copy"});
  retained = std::string(temporary.vector()[1]);
}
std::cout << "owned copy after source=" << retained << '\n';
```

### Output

```text
owned copy after source=copy
```

For a one-cell Note, `temporary.owned()` avoids copying while retaining shared
ownership. The explicit `std::string` above illustrates the independent-copy
choice that also applies to `scalar()`.

## Example 5: Multiple Cells And Absence Reject

```cpp
auto report = [](std::string_view label, const pluss::Note& n) {
  try { (void)n.scalar(); }
  catch (const pluss::Error&) { std::cout << label << ": rejected\n"; }
};

report("scalar two cells", pluss::note::make({"a", "b"}));
report("scalar undefined", pluss::Note::undefined());
```

### Output

```text
scalar two cells: rejected
scalar undefined: rejected
```

## See Also

- [`owned`](owned.md): retain the sole cell with shared ownership.
- [`at`](at.md): borrow one cell by flat position from a multi-cell Note.
- [`is_scalar`](is_scalar.md): test rank zero rather than one-cell cardinality.
- [`vector`](vector.md): borrow every cell in row-major order.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rstrip.html"><span class="label">Previous</span><span class="title">rstrip</span></a>
  <a class="next-link" href="select.html"><span class="label">Next</span><span class="title">select</span></a>
</div>
