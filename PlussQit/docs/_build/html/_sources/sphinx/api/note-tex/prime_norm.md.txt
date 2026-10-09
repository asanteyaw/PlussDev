<!-- pluss-hand-authored -->
# Note::TexAccessor::prime_norm

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::prime_norm() const
```

## Rewrite Trailing `'` Marks As An Explicit `^{\prime...}` Superscript

`prime_norm()` rewrites a run of one, two, or three ASCII apostrophes
immediately after a letter or digit into the equivalent `\prime` superscript:
`x'` becomes `x^{\prime}`, `x''` becomes `x^{\prime\prime}`, and `x'''`
becomes `x^{\prime\prime\prime}`. Use it to normalize derivative notation
before comparing or rendering math source that uses the terse `'` shorthand.

The three lengths are matched longest-first — the three-prime pattern is
applied to the whole source before the two-prime pattern runs, which runs
before the one-prime pattern — so a run of exactly three quotes is rewritten
once as a triple-prime, never left with a leftover single or double match.
A run longer than three apostrophes is not itself a recognized case; only
runs of one, two, or three are matched (a fourth or later apostrophe is
outside any of the three patterns' capture and is copied through as a
literal character). The character immediately before the apostrophe run
must be an ASCII letter or digit — a `'` with no preceding letter/digit
(an opening quotation mark, for instance) is left untouched.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Undefined input remains undefined; Bool, Int64, Float64, and Bytes
receivers are rejected because TeX source is Text.

## Example 1: All Three Lengths In One Cell

```cpp
std::cout << pluss::make_note({"f''' + g'' + h' + x"}).tex().prime_norm()
          << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["f^{\\prime\\prime\\prime} + g^{\\prime\\prime} + h^{\\prime} + x"]
```

`f'''` (three) becomes a triple-prime superscript in one rewrite, not three
separate single-prime rewrites; `x` with no apostrophe is untouched.

## Example 2: A Run Of Four Leaves One Apostrophe Behind

```cpp
std::cout << pluss::make_note({"x''''"}).tex().prime_norm() << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["x^{\\prime\\prime\\prime}'"]
```

The three-prime pass consumes the first three quotes; the fourth is no
longer preceded by a letter or digit (it now follows `}`), so no later pass
matches it and it survives as a literal `'`.

## Example 3: No Primes Present

```cpp
std::cout << pluss::make_note({"no primes here"}).tex().prime_norm()
          << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["no primes here"]
```

## Example 4: A Shape-Preserving Batch

```cpp
std::cout << pluss::make_note({"y''", "no primes"}).tex().prime_norm()
          << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["y^{\\prime\\prime}", "no primes"]
```

## Example 5: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().prime_norm().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().prime_norm();
} catch (const pluss::Error&) {
  std::cout << "Int64: rejected\n";
}
```

### Output

```text
false
Int64: rejected
```

## See Also

- `sup_norm`: braces an already-`^`-prefixed bare
  superscript; this method instead builds the `^{...}` superscript from a
  trailing `'` run.
- `sub_norm`: the `_` counterpart of `sup_norm`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pm_signs.html"><span class="label">Previous</span><span class="title">pm_signs</span></a>
  <a class="next-link" href="refs.html"><span class="label">Next</span><span class="title">refs</span></a>
</div>
