<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 809dcac5238d -->

# Thread::ne

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::ne(double other) const -> Thread
Thread::ne(std::string_view other) const -> Thread
Thread::ne(const char* other) const -> Thread
Thread::ne(const Thread& other) const -> Thread
```

## What It Does

The complement of [`eq()`](eq.md) — `ne()` reports `true` at every position where
`eq()` would report `false`, and vice versa. Same overload shape, same positional
matching, same stringify-then-compare behavior on the string overload for non-`Note`
threads.

## Parameters

**other** (`double`, `std::string_view`, or `const Thread&`)

Same shape as [`eq()`](eq.md).

## Returns

`Thread`
    A new `Bool`-typed thread, same length as `*this`, holding the elementwise
    inequality result — exactly the logical negation of what `eq()` would return for
    the same arguments.

## Examples

Example 1. Compare every value against a scalar.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({72.0, 85.0, 91.0}));
auto out = scores.ne(85.0);
std::cout << out;
```

### Output

```text
0  true
1  false
2  true

ptype: Series
```

The exact negation of [`scores.eq(85.0)`](eq.md)'s `false, true, false`.

Example 2. Compare two threads positionally.

```cpp
auto targets = Thread(pluss::make_series({72.0, 80.0, 95.0}));
auto out_pair = scores.ne(targets);
std::cout << out_pair;
```

### Output

```text
0  false
1  true
2  true

ptype: Series
```

Example 3. String inequality on a `Note` thread.

```cpp
auto names = Thread(pluss::make_note({"Ada", "Bola", "Chen"}));
auto out_names = names.ne("Bola");
std::cout << out_names;
```

### Output

```text
0  true
1  false
2  true

ptype: Series
```

## Notes

- See [`eq()`](eq.md) for the family-wide mechanics: positional matching, the
  length-mismatch throw, and the string-overload's stringify-then-compare behavior
  on non-`Note` threads.

## See Also

- [`Thread::eq`](eq.md): the complementary equality check, and the canonical
  reference for this family's shared mechanics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ndistinct.html">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="normalize.html">
    <span class="label">Next</span>
    <span class="title">normalize</span>
  </a>
</div>
