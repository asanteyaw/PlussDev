# PlussTils User Guide Standard

This standard governs the PlussTils **user guide** (`docs/sphinx/manual/` and
`docs/sphinx/guides/`). It sits above the API reference standard in
`PlussQit/docs/API_Doc_Authoring_Standard.md`. The reference answers "what does
this signature do". The guide answers "how do I use this to get a result, what
do I choose, and what will I see".

A guide page that merely restates the reference in longer sentences fails this
standard.

## 1. Purpose and reader

- The reader is a working programmer who has not used PlussTils before. They
  arrived with a task, not a signature.
- Every page opens with the task the reader is trying to accomplish and ends
  with where to go next.
- The guide covers decisions: which overload to pick, what each option changes,
  what goes wrong when you pick badly, and how to check the result.

## 2. Prose

- Explain *why* before *how*. A paragraph that only names a function is not a
  section.
- Each section makes one point and makes it completely. Where a concept has a
  surprising edge (a zero base that is still a valid base, a stream position that
  is separate from the base), spell out the edge and show it in an example.
- Use the second person and present tense: "you call `rng_seed` once, at the
  start of the run".
- Name the consequences of a choice: what replays, what changes, what is
  reproducible across processes, what is not.
- Do not hand-wave with "etc." or "and so on". If a list matters, enumerate it.
- Do not claim behavior the executable does not demonstrate. If a claim is
  important enough to state, it gets an example, and the example gets output.

## 3. Structure of a guide page

Every guide page contains, in this order:

1. **Opening paragraph:** the task and when a reader needs this page.
2. **Mental model:** the one picture or rule that makes the rest predictable.
3. **Worked tour:** a sequence of examples that build up from the simplest
   correct use to the edge cases, each with a sentence explaining what the next
   example adds.
4. **Decisions:** a short section naming the choices the reader faces and which
   one to pick in which situation.
5. **Pitfalls:** concrete mistakes, each with the output that reveals it.
6. **Where next:** links to the reference page for every symbol the guide used,
   and to the sibling guide pages that build on this one.

## 4. Examples

- Every example is a complete, compilable program. No fragments in a guide.
- Every example that prints has its output copied **verbatim** from the
  permanent executable in `examples/tils/`. Output is never typed by hand.
- Examples are exhaustive along the axes the page covers. If a page covers
  three overloads, each overload has its own example. If a page covers a
  toggle, both states have examples, and the page shows the difference.
- Each example is preceded by a sentence stating what it demonstrates and
  followed by a sentence stating what the output means. The output alone is
  never the explanation.
- Examples use values chosen so the result is checkable by hand. Prefer
  `12345`, `0`, and small arrays to random-looking constants, and say why when
  a constant matters.
- Concurrency examples report properties that are deterministic (a count, a
  uniqueness check), not interleaving-dependent values.

## 5. Verification

- Every illustrative branch a guide page covers is emitted by the permanent
  executable for that family. A branch that is described but not printed by the
  executable is a defect, not an omission.
- The executable is built and run as part of the documentation build. A failing
  check blocks the build.
- Sphinx builds with warnings treated as errors for the guide tree. A warning in
  a guide page is a defect.
- After any change to a public header, the affected guide pages are re-checked
  against the executable before the change is accepted.

## 6. Scope and cross-family discipline

- The guide documents each family on its own terms. Core, Loaders, PlussView,
  and PlussBI are optional shelves. A guide page never states or implies that
  adopting one requires the others.
- Cross-family recipes appear only where the APIs genuinely compose, and they
  are labeled as recipes.
- Internal namespaces (`pluss::bi::detail`, helper types) are not taught.

## 7. Terminology and names

- Use the canonical public spellings from `docs/DOC_PLAN.md`. Do not present
  retired spellings.
- A term is defined once, at first use, in the guide page where it is first
  needed. Later pages link back rather than redefining it.

## 8. Acceptance checklist

A guide page is accepted when all of these hold:

- [ ] The opening states the task and the reader's situation.
- [ ] A mental model appears before the first example.
- [ ] Every example is a complete program.
- [ ] Every printed output is copied verbatim from the permanent executable.
- [ ] Each overload and each toggle the page covers has its own example.
- [ ] Each example is introduced and interpreted in prose.
- [ ] Decisions and pitfalls sections are present and concrete.
- [ ] Every illustrative branch is emitted by the executable.
- [ ] Every symbol used links to its reference page.
- [ ] The page builds in the guide tree with warnings as errors.
