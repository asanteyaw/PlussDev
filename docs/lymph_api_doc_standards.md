# Lymph API Doc Standards

This is the canonical, non-negotiable reference for writing or revising the
handwritten PlussAI / Lymph API docs. `lymph_api_reference_doc_standards.md` is
older background material; where the two disagree, this file wins.

## Working Directives (standing, from review)

These carry the same weight as the rest of the file.

1. **Overload count drives depth.** The more overloads a method has, the more
   prose it needs — behaviour differences, option toggles, which overload to
   reach for — and the more examples it needs. When overloads are genuinely
   dissimilar, dedicate at least one example to each shape.

2. **`## Signature` is the method's own callable surface, nothing else.** Do not
   paste factory or helper declarations from other families into it. If a method
   consumes a type from another family (for example `Model::append` takes a
   `prep::Stage` and a `prep::Route`), state that cross-family dependency in
   prose and link the type's own page; do not imply a false relationship by
   listing unrelated signatures.

3. **Repackage toward `lymph::options::*` and `lymph::reports::*` as you go.**
   Every family has options; several produce reports/results. These are the two
   cross-cutting umbrella namespaces, and they are the **canonical public
   spelling** — docs, signatures, and examples name `lymph::options::X` /
   `lymph::reports::X`. As you document a family, make sure each of its option
   and report types is re-exported there, and switch the prose to that spelling.
   Do this incrementally, never in bulk. End state: root `lymph::` stops
   re-exporting public option/report/result types (they live only in their
   family namespace plus the umbrella); root holds only internal
   family-coordination utilities.

   - **Naming: drop the suffix.** The namespace already supplies the noun, so
     `lymph::reports::CrossValResult` → `lymph::reports::CrossVal`,
     `MetricReport` → `lymph::reports::Metric`, `CompareResult` →
     `lymph::reports::Compare`, `StandardScalerOptions` →
     `lymph::options::StandardScaler`. Short, consistent, one name per concept.
   - **Umbrella = alias facade for these types.** Options and reports are plain
     data. Keep each struct *defined* in its family/leaf header (next to the
     code that uses it) and only `using`-alias it into `lymph::options` /
     `lymph::reports`. Do **not** move the definitions into one header — that
     creates a god-file and include cycles (a family header needs to name its
     own options). "Implementation moves to the canonical scope, not just an
     alias" still applies to *behavioural* types (whole classes/functions), not
     to these data structs — for options/reports the facade alias **is** the
     canonical home, and the family-local name becomes the secondary alias.
   - The family-local spelling (`lymph::prep::StandardScalerOptions`) stays
     valid as the reduced-scope alias, per directive 6.

4. **Add bulk overloads where a method is one-at-a-time.** Methods like
   `Model::append` and `Builder::inject` currently force a long `.append(...)` /
   `.inject(...)` chain for multiple items. When documenting such a method, add
   an efficient bulk overload (`std::initializer_list`, `std::map` /
   `std::unordered_map`, or a range) to the API first — implemented for
   performance, validating the whole batch before mutating — then let the
   signature, prose, and examples cover it.

5. **No classroom or "mental model" framing in headings or prose.** Section
   headings are plain and concrete (`What It Does`, `Parameters`, `Input
   Shapes`, `Practical Examples`, `Errors`). Do not write "Mental Model",
   a classroom-style label, or similar.

6. **Mix alias and full-scope spellings in examples.** Implementations live at
   the fullest scope; the family re-exports a shorter alias. Both must compile.
   Different examples on one page use different spellings so a reader sees both
   and picks their own convention downstream.

## Example Discipline

- Every published output must come from code that was actually run.
- The code block shown in the docs must be the same code that produced the
  advertised output.
- If the user copies the shown code into a file and runs it, the output should
  match what the docs advertise.
- Do not rewrite, shorten, or "clean up" the example after running it and then
  keep the old output. The published code and published output are a locked
  pair.
- Do not invent "expected output" when the page claims the output is verified.
- Do not show loose variables with no provenance. If `truth_labels`,
  `predicted_labels`, `model`, `batch`, or similar names appear, the example
  must show how they were created on the same page unless it is explicitly a
  follow-up example reusing earlier page-local context.
- Every object, method, function, and options page must contain at least three
  practical examples. Add more when overloads, toggles, or materially different
  behaviors need separate treatment. Cover normal
  use and relevant edge behavior. Most explanation should come from end-to-end
  examples: construct realistic inputs, configure the object, run it, and
  inspect the result. Short follow-up examples are encouraged for individual
  methods or toggles once the page has established its page-local context.
- Not every example has to be end to end, but end-to-end examples should be
  the dominant pattern. Use shorter follow-up snippets only when the
  page has already established the data, object, and printing context clearly.
- Place a verified output block immediately beneath every example. Run the
  exact shown program, then copy its output directly under that code block.
  Do this for short examples as well as end-to-end examples.

## Output Presentation

- Under `Practical Examples`, print the object or values directly in code.
- You do not need to explain that `std::cout` prints the output, but the code
  must show the actual printing statements.
- The reader should be able to copy the example into a file, run it, and see
  the same output that appears in the docs.
- The shown output block must be copied from that exact executed code path, not
  from a different helper file or a cleaned-up variant.
- Never show one code sample and paste output from a different program. The
  example code in the page is the verifier artifact.
- Prefer readable output layout. Separate major phases with blank lines so the
  result is scannable instead of jammed together.
- For scalar or key-value style status lines, prefer visible separators:
  - `name = value`
  - `left = value, right = value`
  rather than jammed forms such as `name=value right=value`.
- Do not print full `pluss::Series` object prose by default. Most examples
  should print the carried values through a clearer surface such as a small
  `DataFrame`, `.item<T>()`, or another data-focused display. Print the full
  `Series` object only when shape, dtype, platform, or other metadata is part
  of the lesson.
- Do not publish internal verifier source filenames in public docs. Under
  `Practical Examples`, summarize what the reader is about to see and what the
  examples demonstrate; do not list repo-internal `.cpp` generators.

## Signature Discipline

- Signatures must be public-facing declarations, not pasted internal class or
  struct bodies.
- Do not render signatures as:
  - `class Plan final { ... }`
  - `struct ValidationOptions { ... }`
  - raw namespace-scoped implementation blocks
- Render signatures as public declarations only:

```cpp
lymph::models::Model lymph::models::linear::logistic_regression(
    lymph::LinearFamilyOptions options = {});

const lymph::selection::CrossValOptions&
lymph::selection::Plan::options() const;

lymph::selection::Plan& lymph::selection::Plan::folds(std::int64_t value);

pluss::Series lymph::selection::Plan::scores(...) const;
```

- For option bags, do not dump internal struct definitions in the docs.
- Instead, document them field-by-field in prose:
  - field type + field name
  - meaning
  - default value
  - important effects or constraints

## Public Surface Only

- Do not leak internal names such as `nodej`, `nodek`, `nodeb`,
  `pluss::msharp::rangeview`, or implementation-only helper types in public
  docs or examples.
- Public-facing namespaces are:
  - `pluss::ai::classic`
  - `pluss::ai::nn`
  - `pluss::ai::nlp`
  - `pluss::ai::lymph`
- In Lymph docs, prefer `lymph::*` names when the type is part of the Lymph
  public surface.
- When examples need many option or report names, prefer:
  - `using options = lymph::options;`
  - `using reports = lymph::reports;`
  so examples stay readable without falling back to root `lymph::*Options` or
  root `lymph::*Report` usage.
- If a lower-level public type must be shown, use the public alias family name
  such as `classic::*`, not `nodej::*`.
- In examples, prefer `{...}` over verbose internal range-view construction.

## Example Construction Style

- Prefer concise public factories such as:
  - `pluss::make_series({1.2, 3.4, 2.5}, ...)`
- Avoid unnecessary verbosity like:
  - `pluss::make_series(std::vector<double>{1.2, 3.4, 2.5}, ...)`
- Small synthetic data is acceptable when it shows the API clearly.
- Real data examples are also acceptable, but public docs must not expose local
  absolute filesystem paths.

## Page Structure

- Every method/function page has a required `## What It Does` section,
  immediately after `## Signature` (and any signature-note prose) and
  immediately before `## Parameters`. It is prose, not a parameter-by-
  parameter breakdown: what the call actually does, its effect or the
  value it produces, how it fits into the surrounding API, and anything a
  reader needs before the parameter-level detail makes sense. Parameter
  meaning still belongs in `## Parameters`, not duplicated here.
- Rich prose is required.
- Explain what each parameter or toggle changes in behavior.
- Explain defaults, constraints, tradeoffs, and failure modes where relevant.
- The docs should be written with pedagogical completeness in mind, but do not
  use classroom framing in the
  public prose.
- A careful reader should be able to learn the API deeply from the reference
  alone.
- Each page should be strong enough that a careful reader could build working
  end-to-end usage from that page and its linked neighbors.


## Sidebar Structure

The left sidebar must follow the public family map, not legacy buckets such as
`model`, `split`, or `tune`. It is one persistent, collapsible tree; opening a
page must never replace it with a shorter page-local sidebar. The selected page
is highlighted, and its ancestors are expanded. Do not put literal labels such
as "family" or "subfamily" in that tree: show the public names themselves.

**The tree enumerates navigable API entities only** — families, subfamilies,
objects/types, methods, and free functions — plus exactly one `Introduction`
node per family or subfamily. Nothing else earns a sidebar node. In particular
there are **no `Notes` pages, no standalone single-accessor pages** (for example
a bare `contract` under a lightweight descriptor), **and no object-level
`Introduction` node for an object whose methods are documented on its own
page**.

An object appears in one of two shapes:

1. **Self-contained object** (the common case — every `prep` object: `Stage`,
   `StandardScaler`, the encoders, `FeatureTransformer`, …). One page. Its
   methods are `## Methods` subsections on that page; its role, contract,
   invariants, and postscript material are `## Contract` / `## Notes` sections
   on that same page. No child nodes at all.
2. **Hub object** (`Model`, `Builder` — objects with a large method surface).
   The object page is a hub: an `Introduction` child carries the role prose, and
   each public method is its own page. Only this shape has an `Introduction`
   node, and it never also duplicates what the method pages say.

The manifest encodes the distinction: a `class` symbol with a non-empty
`methods` list is a hub (gets `Introduction` + method pages); an empty `methods`
list is self-contained (single page, no children). `apiref.py wire` / `nav` /
`scaffold` follow that automatically.

**An options page nests under the object it configures.** Give the options
`[[symbol]]` an `owner = "<owning class qualname>"`; `apiref.py` then renders it
as a child of that object in the sidebar and its toctree, not as a sibling at
family level. The indentation carries the "these options belong to this object"
relationship a flat family listing cannot. An options struct shared by several
objects, or owned by none, stays at family level (no `owner`).

The standard navigation rhythm is:

- `lymph`
  - main family
    - subfamily
      - `Introduction`
      - object/type pages
        - (self-contained: one node, with its `Options` page nested beneath)
        - (hub: node + `Introduction` + method pages, `Options` nested beneath)
      - free-function pages
      - family-level options pages (shared or owner-less)

Example:

- `lymph`
  - `models`
    - `builder`
      - `Introduction`
      - `Builder`            *(hub — expands to `Introduction` + method pages)*
    - `linear`
      - `Introduction`
      - `LinearFamilyOptions`
  - `prep`
    - `stage`
      - `Introduction`
      - `Stage`              *(self-contained — single page)*
    - `scaling`
      - `Introduction`
      - `StandardScaler`     *(self-contained — single page)*
        - `StandardScalerOptions`   *(nested: `owner` = `StandardScaler`)*
      - `MinMaxScaler`
        - `MinMaxScalerOptions`

The clickable pages in the sidebar are the navigation contract. A family or
subfamily landing `Introduction` is not ornamental; it explains what that
concern owns before the reader drills into concrete objects.

For a **hub object**, do not lump every method into one giant page. The tree is:

- family (or family → subfamily)
  - type
    - `Introduction`
    - one page per public method, alphabetical

If an in-place and out-of-place pair are the same conceptual method, document
them on one page together — `learn` covers both `learn(...)` and `learn_(...)`.

This is API reference, not a user guide. For a hub object the method page holds
signature, options, behaviour, examples, edge cases, and verified outputs, and
the `Introduction` holds only role prose and cross-cutting notes. For a
self-contained object all of that lives on the one page, sectioned.

The left sidebar itself must remain persistent across clicks. Clicking a family,
subfamily, object, or options page should only change the content panel and the
active-highlight state. It must not replace the full tree with a shorter,
page-local section list.

When sidebar or layout logic is changed, verify the built HTML itself, not just
the markdown source or template source. A source-only check is not sufficient
for navigation work.

## Lymph Families
A family is a broad category used to characterize behaviours, functionalities and, most importantly concerns. The lymph framework is still behind in characterization to have stable family, at least not in the breath of sklearn...yet.

The root lymph is littered with several functionalities that should be grouped into their proper families. The lymph framework should not have much in it's root, most if not all should go into their designated family. We can use sklearn characterizations as a base inspiration to demarcate behaviors and concerns in order to mark out clear families. There might also be interactions between families and those should also be defined clearly. For example, some families may consume others for their work, or some families may output objects or types that exists in other families. So here, there is no closure property(i.e. families are not closed under input and outputs.)

#### Syntax for Families
We should generally have:
lhs = rhs;
On the lhs is the return type or simply the type. The level of granularity should be at most 3. For simplicity, we can alias to reduce it to two. The general syntax should be:
```cpp
using lymph = pluss::ai::lymph;

lymph::family::Type var_name = lymph::family::function(...);

/*Then one more level of granularity is where the implementation actually lives. This is the sub-family. It will be aliased so users can reach it in the main family*/

lymph::family::subfamily::Type var_name = lymph::family::subfamily::function(...);
```
So for users who want utmost explicitness can use the full scope, but for simplicity, and many times users will prefer this, the can just discard the sub-family(though this is where the actually behavioural implementation is) and access the type and functions thorough the main family.
For example:
```cpp
lymph::models::Model m = lymph::models::logistic_regression(...);
```
The level of granularity on lhs is 2, and on the rhs is also 2(we just count how many scope resolution operators there is and call that number the granularity level). So the model family has actually be aliased because there is another level of granularity that users can go to if they want the utmost explicitness
```cpp
using lymph = pluss::ai::lymph;
lymph::models::Model m1 = lymph::models::linear::logistic_regression(...);
```
The rhs has 3 levels of granularity, this should be the most for anything in lymph. Aliases can be done to reduce the granularity to two. But in general, we avoid granularity of 1, like `lymph::Model` because that is what pollutes the root lymph. On very rare occasions, if it is absolutely necessary then we allow one level of granularity.

#### False families
Some of the current 'family' should be consolidated into a larger, more descriptive family. We can use sklearn's families as inspirations. For example:
```cpp
lymph::models::Builder b1 = lymph::models::build({...});
```
Should not be a family, that is `builder` should not be a family on its own. If you check the behaviour, it should be in the `models` family. Another is `validate`, it should be repackaged and put into the `selection` family, just like sklearn has cross_val_score inside model selection
```python
from sklearn.model_selection import cross_val_score
```
cross validation itself is not a family in sklearn, so we treat ours as a subfamily.

#### Family names
Some of the current family names must change under the new repacking and consolidation scheme. We should have `model` become `models` since it now contains the current `model`, `builder`, and related model-worthy subfamilies.

#### Docs
The docs must be built according to the level of granularities too, so as to ensure a consistent rythm with the API
