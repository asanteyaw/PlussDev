<!-- pluss-hand-authored -->
# Note::bool_item

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
bool Note::bool_item() const
```

## Cross From One Bool Lane Into C++ Control Flow

`bool_item()` returns the sole lane of a Bool Note as a native C++ `bool`. It
is the scalar host boundary for a comparison, predicate, or Boolean reduction
that has finished its work in Note space. Use it when a result must drive
`if`, `while`, an assertion, or an API that specifically accepts `bool`.

The method tests **cardinality**, not rank. Any defined Bool Note with
`numel() == 1` is accepted: a rank-zero scalar, shape `[1]`, shape `[1, 1]`,
and shape `[1, 1, 1]` all contain exactly one extractable truth value.
`is_scalar()` is therefore not a prerequisite; it answers the different
question of whether the Note has zero axes.

Extraction is constant-time and allocation-free. The native result is copied,
so it has no lifetime relationship with the Note. Internally stored Boolean
lanes are canonical bytes, and `bool_item()` maps zero to `false` and one to
`true`.

No conversion is attempted. A one-element Text, Bytes, Int64, or Float64 Note
is rejected rather than interpreted for truthiness. A defined empty Note, a
multi-element Bool Note, and an undefined handle are also rejected. Cardinality
is validated before dtype, so inputs without exactly one element fail the
one-element contract regardless of their dtype.

Keep vectorized decisions as Bool Notes while they are still being combined.
Use `&`, `|`, `^`, `~`, `all()`, and `any()` first, then call `bool_item()` once
at the final host boundary. Use [`bools()`](bools.md) instead when every lane is
needed.

## Example 1: Rank Zero And Higher-Rank Singletons

```cpp
const auto scalar = pluss::note::make(false);
const auto rank3 = pluss::note::make(
    std::vector<bool>{true}, std::array<std::int64_t, 3>{1, 1, 1});

std::cout << std::boolalpha
          << "scalar=" << scalar.bool_item()
          << " rank3=" << rank3.bool_item() << '\n';
```

### Output

```text
scalar=false rank3=true
```

The shape `[1, 1, 1]` remains on `rank3`; extraction does not reshape it.

## Example 2: Finish A Closed-Note Predicate Pipeline

```cpp
const bool found = pluss::note::make("chapter").contains("apt").bool_item();
std::cout << std::boolalpha << "pipeline=" << found << '\n';
```

### Output

```text
pipeline=true
```

`contains()` returns a Bool Note. `bool_item()` is appropriate here because
the scalar text input produces exactly one Boolean result.

## Example 3: Empty And Multi-Lane Notes Reject

```cpp
auto report = [](std::string_view label, const pluss::Note& value) {
  try { (void)value.bool_item(); }
  catch (const pluss::Error&) { std::cout << label << ": rejected\n"; }
};

report("bool_item empty", pluss::note::make(std::vector<bool>{}));
report("bool_item multiple", pluss::note::make({true, false}));
```

### Output

```text
bool_item empty: rejected
bool_item multiple: rejected
```

Do not use `bool_item()` as an implicit reduction. Choose `all()` or `any()` so
the aggregation rule is explicit.

## Example 4: Numeric One Is Not Boolean True

```cpp
try {
  (void)pluss::note::make(std::int64_t{1}).bool_item();
} catch (const pluss::Error&) {
  std::cout << "bool_item wrong dtype: rejected\n";
}
```

### Output

```text
bool_item wrong dtype: rejected
```

## Example 5: Undefined Has No Lane To Extract

```cpp
try {
  (void)pluss::Note::undefined().bool_item();
} catch (const pluss::Error&) {
  std::cout << "bool_item undefined: rejected\n";
}
```

### Output

```text
bool_item undefined: rejected
```

## See Also

- [`bools`](bools.md): borrow every native Boolean lane.
- [`is_bool`](is_bool.md): inspect the dtype without throwing.
- [`all`](all.md): reduce all lanes by conjunction and remain in Note space.
- [`any`](any.md): reduce all lanes by disjunction and remain in Note space.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="begin.html"><span class="label">Previous</span><span class="title">begin</span></a>
  <a class="next-link" href="bools.html"><span class="label">Next</span><span class="title">bools</span></a>
</div>
