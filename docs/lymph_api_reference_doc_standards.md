# Lymph API Reference Doc Standards

> **Superseded.** `lymph_api_doc_standards.md` is now the canonical, non-
> negotiable doc-standards file, including the standing "Working Directives"
> section. Keep this file only as background; where the two disagree, the other
> one wins.


This note is a standing authoring contract for Lymph API documentation work.

Use it before and during every Lymph docs pass so the quality does not drift.

## Core Rule

Present Lymph like a serious production ML system, not like a bag of loose code
fragments.

The reader should be able to study the API reference and leave knowing:

- what the object or method does
- when to use it
- what each option changes
- how to run it
- how to inspect its outputs
- what failure modes, edge cases, and quirks matter

The standard is high on purpose: the docs should be rich enough that a diligent
reader could write a book on Lymph by studying them carefully.

## Example Standard

Every practical example must be a real, complete unit, not a detached fragment.

That means:

- do not write a variable and then jump straight to output
- do not assume the reader knows where variables came from
- do not omit how output was printed
- do not use mystery setup hidden elsewhere

Each example should show, explicitly:

- the data setup
- the object construction
- the method call
- the `std::cout` path that produced the shown output

Do not waste prose stating that output is being printed with `std::cout`.

The reader is a C++ programmer. Just print it in the example code naturally.

Bad pattern:

```cpp
lymph::ConfusionTable table =
    lymph::confusion_table(truth_labels, predicted_labels);
```

Good pattern:

```cpp
#include <pluss/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  const pluss::Series truth_labels = pluss::make_series({0.0, 1.0});
  const pluss::Series predicted_labels = pluss::make_series({0.0, 1.0});

  const lymph::ConfusionTable table =
      lymph::confusion_table(truth_labels, predicted_labels);

  std::cout << "== confusion_table ==\n";
  std::cout << "labels=" << table.labels << "\n";
  std::cout << "counts=" << table.counts << "\n";
  std::cout << "normalize=" << static_cast<int>(table.normalize) << "\n";
}
```

## Approved Practical Examples Layout

Use this exact page-local pattern unless there is a strong reason not to:

1. `## Practical Examples`
2. a short setup paragraph for the section
3. `### Example 1`
4. one short paragraph describing what that example demonstrates
5. the full code block
6. `Output`
7. the copied runtime output block
8. `### Example 2`
9. one short paragraph describing what changed and why
10. the code block
11. `Output`
12. the copied runtime output block

This is the approved shape.

The goal is:

- predictable reading flow
- easy scrolling between examples on the same page
- easy comparison of what changed between examples
- clear separation between explanation, code, and output

## Intra-Page Context Reuse Rule

If `Example 2` or a later example on the same page builds directly on earlier
setup, do not mindlessly rewrite the same data-generation code again.

Reuse already-established variables when the context is still on the same page and
the reader can scroll up and confirm them.

Examples:

- if `Example 1` created `prices`, `labels`, `frame`, or `model`
- and `Example 2` is only changing one option or one terminal call
- then reuse the earlier names instead of re-expanding the same setup

This keeps the docs:

- less repetitive
- easier to compare
- more honest about what actually changed

But only reuse context when the prior setup is still clear and local to the same
page.

If the new example would become ambiguous in isolation, restate the setup.

## Numbered Example Rule

Under `Practical Examples`, prefer numbered examples:

- `Example 1`
- `Example 2`
- `Example 3`

Each example should begin with a brief statement of what it is about.

That description should tell the reader why this example exists, for example:

- basic construction
- option toggle
- end-to-end run
- edge case
- interpretation difference
- failure mode
- downstream workflow use

## Output Standard

Do not write "expected output" when the code was not merely hypothetical.

Use actual runtime-verified output copied from executed examples.

Requirements:

- run the example
- capture the real output
- paste the real output under the example
- ensure the printed code path matches the shown output exactly

Most important:

- the example code shown in the docs must be the same code that was actually run
  to produce the shown output

Do not write a cleaner, shorter, or more idealized example and then pair it with
output from a different executed file.

That is misleading.

If a user copies the code block into an editor, builds it, and runs it, they
should get the advertised output.

If page-local context reuse is used for `Example 2` or later, then the earlier
setup shown on the same page must be the exact setup that was actually part of the
executed file for that follow-up example.

If the page shows output, the page must make clear how that output got to the
screen.

If `std::cout` lines are absent, the example is incomplete.

## Options Presentation Standard

Do not present public options objects as raw internal-looking namespace/struct
declarations.

Bad pattern:

```cpp
namespace pluss::ai::lymph::metrics {
  struct ValidationOptions {
    ...
  };
}
```

That is not how the public reference should explain an options object.

Instead:

1. use the object name itself as the heading
2. describe the role of the options bag in prose
3. list each public field in a human-readable field reference
4. state the default value inside the field description where relevant
5. explain what toggling the field changes semantically

Preferred pattern:

- `holdout_fraction`
  description, meaning, and default value
- `folds`
  description, tradeoff, and default value
- `shuffle`
  description, legal contexts, and default value

The goal is to explain the public contract, not to dump internal-looking code.

## Signature Standard

Public signature sections must show public callable surface, not internal class
or namespace implementation layout.

Bad pattern:

```cpp
namespace pluss::ai::lymph::validate {
  class Plan final {
   public:
    explicit Plan(metrics::ValidationOptions options = {});
    ...
  };
}
```

Preferred pattern:

```cpp
lymph::ValidationPlan lymph::validate::cross(
    lymph::ValidationOptions options = {});

const lymph::ValidationOptions& lymph::ValidationPlan::options() const;
lymph::ValidationPlan& lymph::ValidationPlan::folds(std::int64_t value);
pluss::Series lymph::ValidationPlan::scores(...) const;
double lymph::ValidationPlan::score(...) const;
```

Rules:

1. write signatures as callable public surface
2. use the public family or root alias names the reader is meant to call
3. do not dump namespace blocks, class blocks, `public:` labels, or internal
   scaffolding
4. object pages should include a signature section when the public entry surface
   is important to understanding how the object is obtained or used
5. never present a `class` or `struct` definition as a public signature
6. never list an options object by dumping its struct body; explain fields in prose

Template:

```cpp
return_type family_or_type::callable_name(parameters);
return_type ObjectType::method(parameters) const;
```

Examples:

```cpp
lymph::Model lymph::model::logistic_regression(
    classic::LinearFamilyOptions options = {});
pluss::Series lymph::Model::infer(
    const pluss::table::DataFrame& frame,
    const lymph::DataFrameBatchOptions& data_options) const;
```

## Public Namespace Standard

Public docs and examples must not expose internal coded framework names such as:

- `nodej`
- `nodek`
- `nodeb`

Use the public names instead:

- `pluss::ai::classic`
- `pluss::ai::nn`
- `pluss::ai::nlp`

or use the public `lymph::...` aliases when the contract is intentionally
re-exported at the Lymph level.

If an example needs a namespace alias, prefer:

```cpp
namespace lymph = pluss::ai::lymph;
namespace classic = pluss::ai::classic;
```

## Family Framing Standard

Document Lymph by real public families.

If the public surface is:

- `lymph::metrics::*`
- `lymph::validate::*`
- `lymph::model::*`
- `lymph::tune::*`

then the docs should frame those as families directly.

Do not create a misleading parent-family story that makes one real public family
look like a sub-branch of another.

Cross-links are fine.

Misframing the public family structure is not.

## Multiple Examples Rule

Every object, method, function, and options page requires at least three
practical examples. One tiny example is never sufficient for a completed API
reference page.

For each important object or method, prefer multiple examples under practical
examples, including some mix of:

- normal usage
- end-to-end usage
- option toggles
- edge cases
- quirks
- failure modes
- interpretation-oriented examples

At least one example should usually show the normal path.

For bigger objects, especially in ML workflow families, prefer several examples,
most of them end-to-end where sensible.

When possible, organize them so the examples escalate naturally:

1. baseline usage
2. one important option or variation
3. end-to-end or workflow-level usage
4. edge case or quirk

## End-to-End Bias

Lymph is not like a tiny one-shot helper. It is a modeling system.

So examples should often reflect real flow:

1. create or load data
2. define options
3. build model or workflow object
4. run training, validation, tuning, or reporting
5. print the result
6. explain what the result means

Use fragment-only examples sparingly and only when the narrow focus is genuinely
helpful.

## Data Construction Rule

For small synthetic examples, generate data directly in the example.

Preferred:

```cpp
const pluss::Series y = pluss::make_series({0.0, 1.0, 1.0, 0.0});
```

Avoid unnecessary verbosity such as:

```cpp
pluss::make_series(std::vector<double>{0.0, 1.0, 1.0, 0.0});
```

unless the API surface truly requires the more explicit form.

For small doc examples:

- prefer `pluss::make_series({...})`
- prefer direct literals where clear
- avoid bulky setup that adds nothing

For real-data examples:

- Lymph demo data is allowed
- do not expose internal full filesystem paths in public docs
- prefer neutral public-facing examples such as
  `pluss::table::DataFrame::load("data.csv")`

## Prose Standard

The prose should be rich, deliberate, and instructional.

Do not merely name parameters. Explain them.

For each important option or toggle, explain:

- what it controls
- why it exists
- what changes when it is toggled
- when to prefer one setting over another
- any cost, tradeoff, or interpretation consequence

The goal is not terse API stubs. The goal is guided understanding.

## Internal Leak Rule

Do not leak internal naming or internal code surfaces into public docs unless the
public API truly exposes them.

Examples:

- prefer `pluss::RangeView` over `pluss::msharp::rangeview` in public-facing
  signatures or prose
- in examples, prefer direct braces such as `{...}` instead of exposing internal
  rangeview machinery

The public docs should read like a public API, not like an internal implementation
tour.

## Verification Rule

Every serious example should be verified before publication.

Verification means:

- the code compiles
- the code runs
- the output shown in docs comes from that run
- the code block in the docs is the exact code from that run, not a stylized
  substitute
- the prose matches what the API actually does
- the output block sits directly under the example it came from

If a doc example has not been run, treat it as untrusted until verified.

## Writing Style Guardrails

- prefer explicit type names when they improve pedagogy
- avoid unexplained `auto` in example-first pages
- keep examples self-contained enough to read in isolation
- use short helper-free examples unless the helper itself is the subject
- do not hide essential setup in unnamed external context
- explain what is being printed and why

## Review Checklist

Before considering a Lymph docs page done, check:

- does the example show where every important variable came from?
- does it show how the output was printed?
- was the output actually run and copied?
- are there multiple examples where the object deserves them?
- is there at least one end-to-end example when appropriate?
- does the prose explain option effects and tradeoffs?
- did any internal-only naming leak into the public page?
- is the code using concise public-facing syntax where possible?
- would a serious reader be able to reproduce and understand the example without
  guessing?

If the answer to any of these is no, the page is not ready.
