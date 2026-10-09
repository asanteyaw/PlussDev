# `pluss::ai::procedure::step`

The `nlp` adapter overload: wraps an `nlp::Workflow` as a persistable
learn/infer/assess step accepting text, `pluss::Note`, or `Doc`-vector
inputs depending on `RunOptions::mode`.

## Signature

```cpp
pluss::ai::Step pluss::ai::procedure::step(std::string name, std::string input_name, std::string output_name, nlp::Workflow workflow, nlp::TrainOptions train_options = {}, nlp::AssessOptions assess_options = {});
```

## What It Does

Builds a `Learn`/`Infer`/`Assess`-capability step that calls the
workflow's own `learn`/`assess`/`infer` depending on
[`RunOptions::mode`](../core/RunOptions.md). In `Learn`/`Assess` mode,
the input may be a text vector, a `pluss::Note`, or a
`std::vector<nlp::Doc>`; in the default `Infer` mode, it may be a single
`std::string`, a text vector, or a `pluss::Note`. The step is always
persistable, recording `train_options`/`assess_options` into its audit
fields.

## Parameters

`std::string name`, `std::string input_name`, `std::string output_name`
: The step's name and its one input/output port.
`nlp::Workflow workflow`
: The workflow to wrap.
`nlp::TrainOptions train_options`
: Passed through to `learn`. Defaults to `TrainOptions{}`.
`nlp::AssessOptions assess_options`
: Passed through to `assess`. Defaults to `AssessOptions{}`.

## Returns

A [`Step`](../core/Step.md) value.

## Errors

- Throws if `input_name` or `output_name` is empty.
- At run time, throws if the resolved input artifact's type does not
  match any of the shapes the active mode accepts.

## Practical Examples

### Example 1

A single-stage workflow run in the default `Infer` mode over a text
vector.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::sentencize()},
  };
  const std::vector<std::string> texts = {"Procedure runs NLP as one deployable step."};

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("nlp", "text", "docs", wf),
  });
  const ai::ProcedureResult out = proc.run({ai::procedure::input("text", texts)});
  std::cout << "docs=" << out.get<std::vector<nlp::Doc>>("docs").size() << "\n";
  return 0;
}
```

Output

```text
docs=1
```

### Example 2

Every `nlp` step is persistable by construction, with no extra setup.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const ai::Step s = ai::procedure::step("nlp", "text", "docs", wf);

  std::cout << "persistable=" << s.persistable() << "\n";
  return 0;
}
```

Output

```text
persistable=1
```

### Example 3

Chaining an `nlp` step into a sink: documents produced from two input
texts flow through unchanged.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const std::vector<std::string> texts = {"one", "two"};

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("nlp", "text", "docs", wf),
      ai::procedure::step("store", ai::procedure::sink("procedure.demo.docs")),
  }, {
      ai::procedure::link("nlp", "store"),
  });
  const ai::ProcedureResult out = proc.run({ai::procedure::input("text", texts)});
  std::cout << "stored=" << out.get<std::vector<nlp::Doc>>("store").size() << "\n";
  return 0;
}
```

Output

```text
stored=2
```

## See Also

- [`nlp`](../../plussai_nlp.md) -- the framework `Workflow` belongs to.
- [`lymph`](../lymph/step.md) / [`ice`](../ice/step.md) -- the sibling
  adapters for other engines.
