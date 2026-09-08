<!-- internal authoring standard for handwritten API references -->

# API Documentation Authoring Standard

This is the standing quality bar for handwritten PlussQit API references. It
applies to free functions, instance methods, static methods, operators, and
accessor methods.

## Prose Must Teach The Complete Contract

A page is not complete when it merely paraphrases the function name. Its prose
must give a reader enough understanding to choose the operation correctly and
predict its behavior without reading the implementation.

For every operation, determine and explain:

- why the operation exists and the problem it solves
- when it is the right choice, and when a neighboring operation is more precise
- every public call form and what is different about each overload
- the meaning, default, and interaction of every parameter
- input rank, shape, broadcasting, axis, dtype, timezone, and precision rules
- output type, rank, shape, ordering, ownership, and metadata
- mutation or allocation behavior
- absent-value, boundary, tie, overflow, and empty-input behavior
- error conditions and whether validation occurs before producing a result
- performance characteristics that affect how callers should use it

Do not turn implementation defects into usage advice. Verify the behavior first;
if it violates the intended contract or authoritative semantics, fix it before
documenting it.

## Examples Are A Behavioral Inventory

Example count is determined by the surface area, not by a fixed quota. Every
materially distinct way to use an operation needs an illustrative example.

The example inventory should include every applicable dimension:

- each overload and accepted call-site spelling
- scalar, one-dimensional, and higher-rank input
- shape preservation, flattening, reduction, or broadcasting
- default arguments and each behavior-changing option
- positive, negative, zero, inclusive, and exclusive boundaries
- timezone and daylight-saving behavior for temporal operations
- full precision when a compact printer hides meaningful digits
- absent values and empty shapes
- mutation versus out-of-place behavior
- deterministic or seeded behavior for random operations
- representative failures, including invalid rank, shape, dtype, domain, or
  parameter combinations

A simple query may need three examples. A function with several overloads and
behavioral branches may need ten or more. The page is complete only when the
examples collectively cover the public contract.

## Output Integrity

- Compile and run every public example that claims output.
- Place output immediately below the code that produced it.
- Copy the exact output; never substitute a hand-simplified representation.
- If the ordinary printer hides the behavior being taught, use a projection or
  formatter that exposes it.
- Do not invent fixed output for wall-clock or nondeterministic operations. Use
  seeded inputs or verify stated invariants instead.
- Keep a permanent reference executable for substantial documentation families.

## Authoritative Behavioral Checks

For datetime semantics, consult the current official `pandas.Timestamp` and
datetime-array documentation and source as an external correctness reference.
Use it to check replacement, offsets, timezone transitions, precision,
comparisons, and absent-instant behavior. The public PlussQit prose should teach
PlussQit directly rather than advertise or compare external APIs.

An external reference is evidence, not an excuse to copy names that do not fit
the native PlussQit surface. Resolve intentional differences explicitly in the
implementation review.

If you find an (or what appears to be an) implementation bug or even a heavily non-performant implementation, fix it first of correctness, then performance. For performance, ensure full vectorization, batching, caching or anything that will materially improve the performance of the function or method, including the usage of native kernels/ops.

## Public Presentation

- Use canonical public names; never leak internal namespace or adapter names.
- Keep public callable names to at most three underscore-separated semantic
  components. Prefer short domain terms such as `ticks()` over implementation-
  shaped names such as `ticks()`. Descriptive internal kernel
  names are exempt. Rename directly rather than retaining a compatibility alias.
- Put the return type first in C++ signatures.
- Present option/configuration structures as tables with at least field,
  meaning, and default columns. Add an applicability column when fields affect
  only some modes. Do not use an aggregate-initializer block as a substitute
  for explaining an option contract; reserve construction syntax for runnable
  examples that configure an actual call.
- Keep one canonical page and sidebar entry per public operation.
- Avoid duplicate aliases and compatibility spellings in the teaching surface.
- Give each accessor or factory family its own landing page and per-operation
  pages.
- Order operation pages A-Z in the landing toctree, browser sidebar, and
  family-aware previous/next chain.
- Wire every page into its landing toctree, `api_nav.js`, and one family-aware
  previous/next chain.
- Do not name comparison libraries in public method prose or examples.

## Completion Checklist

Before declaring a page or family complete:

1. Audit declarations, dispatch, kernels, tests, and neighboring operations.
2. Correct implementation defects and inconsistent overload behavior.
3. Add a permanent verifier that exercises every documented branch.
4. Run the verifier and copy exact outputs into the matching examples.
5. Build the HTML documentation and check landing links, sidebar membership,
   active-page selection, and previous/next controls.
6. Confirm no generated page overwrote the handwritten source and no new build
   warnings were introduced.
