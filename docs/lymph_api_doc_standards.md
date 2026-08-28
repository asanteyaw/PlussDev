# Lymph API Doc Standards

Use this note as the non-negotiable reference while writing or revising the
handwritten PlussAI/Lymph API docs.

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
- Every object and options page must contain several examples whenever the
  surface has more than one meaningful operation or decision. Cover normal
  use and relevant edge behavior. Most teaching should come from end-to-end
  examples: construct realistic inputs, configure the object, run it, and
  inspect the result. Short follow-up examples are encouraged for individual
  methods or toggles once the page has established its page-local context.
- Not every example has to be end to end, but end-to-end examples should be
  the dominant teaching pattern. Use shorter follow-up snippets only when the
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
  - `pluss::ai::nets`
  - `pluss::ai::nlp`
  - `pluss::ai::lymph`
- In Lymph docs, prefer `lymph::*` names when the type is part of the Lymph
  public surface.
- If a lower-level public type must be shown, use the public alias family name
  such as `classic::*`, not `nodej::*`.
- In examples, prefer `{...}` over verbose internal range-view construction.

## Example Construction Style

- Prefer concise public factories such as:
  - `pluss::make_series({1.2, 3.4, 2.5}, ...)`
- Avoid unnecessary verbosity like:
  - `pluss::make_series(std::vector<double>{1.2, 3.4, 2.5}, ...)`
- Small synthetic data is acceptable when it teaches the API clearly.
- Real data examples are also acceptable, but public docs must not expose local
  absolute filesystem paths.

## Page Structure

- Rich prose is required.
- Explain what each parameter or toggle changes in behavior.
- Explain defaults, constraints, tradeoffs, and failure modes where relevant.
- The docs should be written with pedagogical completeness in mind, but do not
  literally say "teach", "teaching", or similar classroom framing in the
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

The standard navigation rhythm is:

- `lymph`
  - main family
    - subfamily
      - `Introduction`
      - object/type pages
      - options pages

Example:

- `lymph`
  - `models`
    - `builder`
      - `Introduction`
      - `Builder`
    - `linear`
      - `Introduction`
      - `LinearFamilyOptions`
  - `prep`
    - `stage`
      - `Introduction`
      - `Stage`
    - `scaling`
      - `Introduction`
      - `StandardScaler`
      - `StandardScalerOptions`

The clickable pages in the sidebar are the navigation contract. A family or
subfamily landing page is not ornamental; it explains what that concern owns
before the reader drills into concrete objects.

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
