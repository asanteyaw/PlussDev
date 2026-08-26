<!-- internal authoring note for handwritten Series docs -->

# Series Doc Authoring Standard

This note records the standing style and quality bar for the handwritten
`Series` API reference work in `PlussQit/docs/sphinx/api/series`.

Use this note as the working standard for every remaining `Series` page unless
the user explicitly changes direction.

## Public Naming Rules

- Public-facing names are `Series`, `Note`, and `Timeline`.
- Do not surface internal coded names like `coden`, `codem`, or `codeh` in the
  prose unless a private/internal implementation note is absolutely unavoidable.
- Do not teach through comparisons to other public libraries or APIs.
- Do not mention names like `torch`, `numpy`, or similar external surfaces in
  these public docs.
- Prefer public aliases over internal type spellings in signatures and
  parameter descriptions when the doc can do so cleanly.
- Avoid exposing `pluss::msharp` in example code. If a public call site can be
  expressed without it, do that.
- When printing a `Series`, prefer `pluss::series::print(...)` over the older
  flat helper spelling.

## Prose Standard

- Write with the goal of teaching the reader how to use `Series` in real
  projects, not just documenting that a method exists.
- Explain each parameter, including every toggle and overload branch.
- State what each toggle changes:
  shape, dtype, broadcasting behavior, axis semantics, mutation behavior,
  reduction behavior, gradient behavior, and failure behavior where relevant.
- Distinguish clearly between:
  reduction vs transform,
  in-place vs out-of-place,
  comparison vs reduction,
  value-returning vs index-returning,
  global vs axis-wise behavior.
- Prefer richer prose that teaches the mental model behind the method, not just
  a terse definition.
- Add explicit "Gotchas And Failure Modes" sections where useful.
- Add a short "Teaching Summary" section on load-bearing methods.
- If a method has quirks, say so directly and concretely.

## Example Standard

- Every example must be verified against the real implementation.
- The output goes directly under the example that produced it.
- Use several examples in incremental fashion.
- For larger surfaces with many overloads or richer behavior, use more examples.
  A sophisticated page may need 7-10 examples.
- Escalate examples from simple to advanced:
  plain case,
  axis/toggle variation,
  broadcasting variation,
  negative dims,
  keepdim behavior,
  scalar vs Series behavior,
  in-place behavior,
  gradient behavior,
  tie behavior,
  error/failure behavior.
- Do not include an example unless its output is known and verified.

## Example Syntax Rules

- Prefer brace-based `make_series` calls:
  `pluss::make_series({1, 2, 3}, {3})`
  not `pluss::make_series(std::vector<double>{1, 2, 3}, {3})`.
- For 2D and nested examples, prefer plain brace lists when supported:
  `pluss::make_series({{1.0, 2.0}, {3.0, 4.0}})`.
- If a parameter accepts range-like input, prefer ordinary brace usage or plain
  containers in the example instead of explicitly constructing internal
  `RangeView`/`msharp` wrappers.
- Avoid teaching low-level internal construction patterns that would encourage
  users to program against internals.

## Verification Discipline

- Back every public example with a compiled-and-run demo when possible.
- If a page is rewritten substantially, update or extend the backing demo first
  if the current coverage is too thin.
- Use the real outputs from those demos in the page.
- Re-run:
  `docs/scripts/validate_doc_examples.py`
  and the PlussQit Sphinx build after substantial page batches.
- Keep previously known build warnings separate from any new regressions.

## Page Structure Preference

For load-bearing `Series` methods, prefer this shape:

1. Signature
2. What It Does
3. Overload Map, if needed
4. Parameters
5. Returns
6. Gradient, if relevant
7. Examples
8. Gotchas And Failure Modes
9. Teaching Summary
10. See Also

Not every page needs every section, but this is the default standard.

## Tone

- Clear, direct, and teacherly.
- Richer than terse reference prose, but still precise.
- No fluff.
- No internal-engineering leakage unless the behavior cannot be explained
  honestly without it.
