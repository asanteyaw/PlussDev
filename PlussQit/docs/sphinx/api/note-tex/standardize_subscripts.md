<!-- pluss-hand-authored -->
# NoteTexAccessor::standardize_subscripts

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::standardize_subscripts() const
```

## Brace Every Bare Subscript

`standardize_subscripts()` wraps every unbraced subscript — `_x`, a run of
one or more ASCII letters/digits, or `_\alpha`, a single control word —
in explicit braces: `_{x}`, `_{\alpha}`. Use it before further TeX
processing that only recognizes the braced form, or to normalize source
that mixes `_x` and `_{x}` for the same conceptual subscript.

**The match is greedy over the whole following alnum run, not just one
character.** `x_1b` becomes `x_{1b}`, not `x_{1}b` — there is no boundary
between a digit and a following letter, so `1b` is captured as one
subscript. Write `x_1 b` (a space) if the digit and the letter are meant to
be a script and separate following text.

A subscript already wrapped in braces — `_{already}` — is left untouched;
there is no second pass to normalize its interior. A control word subscript
(`_\alpha`) captures the whole word, matching how `\alpha` is one token even
though it is not itself alphanumeric before the backslash.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Undefined input remains undefined; Bool, Int64, Float64, and Bytes
receivers are rejected because TeX source is Text.

## Example 1: Bare, Control-Word, And Already-Braced Forms

```cpp
std::cout << pluss::make_note({"x_1 + y_ab + z_\\alpha + w_{already}"})
              .tex().standardize_subscripts()
          << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["x_{1} + y_{ab} + z_{\\alpha} + w_{already}"]
```

## Example 2: The Greedy Run Captures Digit And Letter Together

```cpp
std::cout << pluss::make_note({"a_1b_2", "no subs here"})
              .tex().standardize_subscripts()
          << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a_{1b}_{2}", "no subs here"]
```

`a_1b_2` has two subscript starts (`_1b` and `_2`), each captured as far as
the run of alnum characters extends — `1b` together, then `2` alone.

## Example 3: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().standardize_subscripts().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().standardize_subscripts();
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

- `standardize_superscripts`: the identical bracing rule applied to `^`
  instead of `_`.
- `standardize_primes`: rewrites trailing `'` marks into an explicit `^{...}`
  superscript, a related but distinct normalization.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="standardize_limits.html"><span class="label">Previous</span><span class="title">standardize_limits</span></a>
  <a class="next-link" href="standardize_superscripts.html"><span class="label">Next</span><span class="title">standardize_superscripts</span></a>
</div>
