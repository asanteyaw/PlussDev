# PlussAI sklearn-style API reference plan

I checked the broader pattern, not just the one class page: scikit-learn's [API Reference root](https://scikit-learn.org/stable/modules/classes.html), a module page like [`sklearn.ensemble`](https://scikit-learn.org/stable/api/sklearn.ensemble.html), a class page like [`AdaBoostClassifier`](https://scikit-learn.org/stable/modules/generated/sklearn.ensemble.AdaBoostClassifier.html), a function page like [`train_test_split`](https://scikit-learn.org/stable/modules/generated/sklearn.model_selection.train_test_split.html), and sklearn's own contributor note that their API docs are code-adjacent and generated from API metadata/docstrings ([contributing docs](https://scikit-learn.org/stable/developers/contributing.html)). I also checked PyTorch's [Reference API root](https://docs.pytorch.org/docs/stable/pytorch-api.html).

My recommendation is: do not build PlussAI docs as one-page-per-method like PlussMic. For AI, the right shape is sklearn-style, meaning one page per public object.

## Terminology

Use this hierarchy consistently:

- `repo` = the whole documentation universe
- `framework` = `classic`, `nets`, `nlp`, `lymph`, `ice`, `procedure`
- `family` = branches inside a framework such as `model`, `split`, `prep`,
  `metrics`, `validate`, `spotcheck`, `io`
- `object` = the actual class/type/function page such as `Model`,
  `LabeledBatch`, `holdout_split`, `Procedure`

So `classic`, `lymph`, and `ice` are not families. They are frameworks.

## What To Copy

- A root API reference page grouped by public framework, like sklearn groups by module.
- A framework page per namespace, like `classic`, `nets`, `nlp`, `lymph`, `ice`, `procedure`.
- A family page inside each framework, such as `split`, `prep`, `metrics`, `model`, `validate`.
- One page per class/type/function.
- Class pages should contain constructor signature, summary, parameters, attributes, methods, examples, see-also, version notes, and links to user-guide material.
- Function pages should contain signature, summary, parameters, returns, examples, and see-also.
- Framework pages should contain short prose plus family tables, not raw file dumps.
- Family pages should contain short prose plus object tables, not raw file dumps.
- Public docs should use only `classic`, `nets`, `nlp`; never present `nodej`, `nodek`, `nodeb`.

## Recommended Shape

- `PlussAI/docs/sphinx/api_reference.md`
- `PlussAI/docs/sphinx/api/plussai_root.md`
- `PlussAI/docs/sphinx/api/classic.md`
- `PlussAI/docs/sphinx/api/nets.md`
- `PlussAI/docs/sphinx/api/nlp.md`
- `PlussAI/docs/sphinx/api/lymph.md`
- `PlussAI/docs/sphinx/api/ice.md`
- `PlussAI/docs/sphinx/api/procedure.md`
- `PlussAI/docs/sphinx/api/classic/split.md`
- `PlussAI/docs/sphinx/api/classic/prep.md`
- `PlussAI/docs/sphinx/api/lymph/model.md`
- `PlussAI/docs/sphinx/api/lymph/validate.md`
- `PlussAI/docs/sphinx/api/classic/AdaBoostLikeThing.md`
- `PlussAI/docs/sphinx/api/lymph/Model.md`
- `PlussAI/docs/sphinx/api/lymph/Workflow.md`
- `PlussAI/docs/sphinx/api/procedure/Procedure.md`
- `PlussAI/docs/sphinx/api/classic/train_test_split.md`

For classes, keep methods on the same page under anchors, like sklearn. Do not split every method into its own page unless the object is enormous and truly needs a secondary methods page.

## Scalable Authoring Model

- Handwrite the prose.
- Do not hand-maintain nav, symbol indexes, or signature drift.
- Keep a small manifest per framework, for example `api/classic/_manifest.yml`.
- Inside it, group entries by family.
- The manifest should declare framework, family, public objects, kind (`class`, `function`, `enum`, `report`, `options`), display name, namespace, source header, and category.
- A small internal script should generate only scaffolding:
  - family object tables
  - global symbol index
  - prev/next links
  - missing-doc coverage report
  - signature drift warnings against headers
- The script should not generate user-facing prose.
- This is the scalable part: prose stays curated, structure stays automated.

## Page Contract

- Every class page should follow one fixed contract.
- `Summary`
- `Signature`
- `Parameters`
- `Attributes`
- `Methods`
- `Examples`
- `See Also`
- `Notes`
- `Version History`

- Every function page should follow one fixed contract.
- `Summary`
- `Signature`
- `Parameters`
- `Returns`
- `Examples`
- `See Also`
- `Notes`
- `Version History`

- Every options/report page should follow one fixed contract.
- `Summary`
- `Shape` or `Fields`
- `Used By`
- `Examples`
- `Notes`

## Important Design Choice

- PlussMic can remain method-first because its surface is dense and accessor-heavy.
- PlussAI should be object-first because users think in estimators, workflows, batches, modules, procedures, and reports.
- So `lymph::Model` gets one authoritative page.
- `lymph::Workflow` gets one authoritative page.
- `classic::LabeledBatch` gets one authoritative page.
- `procedure::Procedure` gets one authoritative page.

Navigation, however, should still narrow in this order:

- repo
- framework
- family
- object

## Implementation Plan

1. Freeze the public framework taxonomy: `classic`, `nets`, `nlp`, `lymph`, `ice`, `procedure`.
2. Define doc kinds: `class`, `function`, `enum`, `options`, `report`.
3. Define family taxonomies inside each framework.
4. Create the manifest format and validation script.
5. Build framework landing pages with family tables.
6. Build family landing pages with object tables.
7. Build the first canonical object pages.
   `classic` batch/split/model-spec core
   `nets` module core
   `nlp` workflow/doc core
   `lymph` model/workflow/validate core
   `ice` model/coach/source core
   `procedure` procedure/step/artifact core
8. Add signature-drift checks so docs stay honest as APIs grow.
9. Add coverage checks so any new public symbol fails CI unless categorized as documented or intentionally hidden.

## Narrowing Build Order

The docs build should support a narrowing order like this:

1. repo-wide API reference build
2. framework-wide API reference build
3. family-only API reference build

That means the build system should let you run:

```bash
# whole repo
python3 docs/scripts/build_unified_docs.py html --site all

# one framework/site
python3 docs/scripts/build_unified_docs.py html --site plussai

# one framework inside PlussAI
python3 docs/scripts/build_unified_docs.py html --site plussai --framework classic

# one family inside one framework
python3 docs/scripts/build_unified_docs.py html --site plussai --framework classic --family split
```

The current script only supports site-level narrowing. So framework and family are
not just naming concerns; they should become first-class CLI filters in the docs
builder.

## Linking Rule

Every level should link upward and downward:

- repo API reference links to framework pages across the whole repo
- PlussAI API reference links to its framework pages
- each framework page links to its family pages
- each family page links to its object pages
- each object page links back to both its family and framework

So the docs can be built and navigated in the same narrowing order:

- repo -> framework -> family -> object

My strongest recommendation is this: handwritten prose, manifest-driven structure, object-first pages, public-family naming only. That gives you sklearn-style usability without falling back into generated sludge.

If you want, I can next draft the exact manifest schema and the first three canonical page templates for `lymph::Model`, `classic::LabeledBatch`, and `procedure::Procedure`.

## Sidebar-Oriented Hierarchy

This is the hierarchy I would aim for so the left sidebar reads like a sklearn-style
API reference rather than a file dump.

```text
PlussAI/docs/sphinx/
├── index.md
├── api_reference.md
├── api/
│   ├── plussai_root.md
│   ├── classic.md
│   ├── nets.md
│   ├── nlp.md
│   ├── lymph.md
│   ├── ice.md
│   ├── procedure.md
│   │
│   ├── classic/
│   │   ├── _manifest.yml
│   │   ├── split.md
│   │   ├── prep.md
│   │   ├── metrics.md
│   │   ├── model.md
│   │   ├── tune.md
│   │   └── io.md
│   │   │
│   │   ├── core/
│   │   │   ├── FeatureBatch.md
│   │   │   ├── LabeledBatch.md
│   │   │   ├── ModelSpec.md
│   │   │   ├── TransformSpec.md
│   │   │   └── Flow.md
│   │   ├── split/
│   │   │   ├── holdout_split.md
│   │   │   ├── crossval_plan.md
│   │   │   ├── stratified_crossval_plan.md
│   │   │   ├── group_crossval_plan.md
│   │   │   └── ts_crossval_plan.md
│   │   ├── prep/
│   │   │   ├── StandardScaler.md
│   │   │   ├── MinMaxScaler.md
│   │   │   ├── OneHotEncoder.md
│   │   │   └── ...
│   │   ├── metrics/
│   │   │   ├── confusion_table.md
│   │   │   ├── classification_summary.md
│   │   │   ├── regression_metrics.md
│   │   │   └── ...
│   │   ├── models/
│   │   │   ├── linear.md
│   │   │   ├── tree.md
│   │   │   ├── kernel.md
│   │   │   ├── bayes.md
│   │   │   └── neighbor.md
│   │   ├── tune/
│   │   │   ├── TuneReport.md
│   │   │   ├── sweep.md
│   │   │   ├── tune.md
│   │   │   └── probe.md
│   │   └── io/
│   │       ├── save.md
│   │       ├── load.md
│   │       └── FlowBundle.md
│   │
│   ├── nets/
│   │   ├── _manifest.yml
│   │   ├── core.md
│   │   ├── activations.md
│   │   ├── losses.md
│   │   ├── embeddings.md
│   │   ├── rnn.md
│   │   ├── attention.md
│   │   ├── cnn.md
│   │   └── optim.md
│   │   │
│   │   ├── core/
│   │   │   ├── Module.md
│   │   │   ├── ParameterRef.md
│   │   │   └── Snapshot.md
│   │   ├── activations/
│   │   │   ├── Activation.md
│   │   │   └── ...
│   │   ├── losses/
│   │   │   ├── Loss.md
│   │   │   └── ...
│   │   ├── embeddings/
│   │   │   ├── Embedding.md
│   │   │   └── ...
│   │   ├── rnn/
│   │   │   ├── RNN.md
│   │   │   ├── GRU.md
│   │   │   └── LSTM.md
│   │   ├── attention/
│   │   │   ├── MultiHeadAttention.md
│   │   │   └── Transformer.md
│   │   ├── cnn/
│   │   │   ├── Conv1d.md
│   │   │   ├── Conv2d.md
│   │   │   └── Conv3d.md
│   │   └── optim/
│   │       ├── Optimizer.md
│   │       └── ...
│   │
│   ├── nlp/
│   │   ├── _manifest.yml
│   │   ├── core.md
│   │   ├── tokenize.md
│   │   ├── vectorize.md
│   │   ├── embed.md
│   │   ├── runtime.md
│   │   └── io.md
│   │   │
│   │   ├── core/
│   │   │   ├── Doc.md
│   │   │   ├── Workflow.md
│   │   │   └── WorkflowReport.md
│   │   ├── tokenize/
│   │   │   ├── tokenize.md
│   │   │   └── ...
│   │   ├── vectorize/
│   │   │   ├── Mapper.md
│   │   │   ├── Hasher.md
│   │   │   └── ...
│   │   ├── embed/
│   │   │   ├── Word2Vec.md
│   │   │   ├── Encoder.md
│   │   │   └── ...
│   │   ├── runtime/
│   │   │   ├── Runtime.md
│   │   │   └── ...
│   │   └── io/
│   │       ├── save.md
│   │       └── load.md
│   │
│   ├── lymph/
│   │   ├── _manifest.yml
│   │   ├── core.md
│   │   ├── prep.md
│   │   ├── validate.md
│   │   ├── select.md
│   │   ├── spotcheck.md
│   │   ├── feature.md
│   │   ├── metrics.md
│   │   └── workflow.md
│   │   │
│   │   ├── core/
│   │   │   ├── Model.md
│   │   │   ├── Workflow.md
│   │   │   ├── Policy.md
│   │   │   └── Payload.md
│   │   ├── prep/
│   │   │   ├── prep.md
│   │   │   ├── StandardScaler.md
│   │   │   └── ...
│   │   ├── validate/
│   │   │   ├── ValidationOptions.md
│   │   │   ├── Plan.md
│   │   │   ├── cross.md
│   │   │   ├── time.md
│   │   │   └── holdout.md
│   │   ├── select/
│   │   │   ├── Builder.md
│   │   │   ├── CompareReport.md
│   │   │   └── ...
│   │   ├── spotcheck/
│   │   │   ├── Suite.md
│   │   │   ├── suite.md
│   │   │   └── ...
│   │   ├── feature/
│   │   │   ├── FeaturePickOptions.md
│   │   │   ├── Result.md
│   │   │   └── ...
│   │   ├── metrics/
│   │   │   ├── Threshold.md
│   │   │   ├── CrossValidationReport.md
│   │   │   └── ...
│   │   └── workflow/
│   │       ├── feature.md
│   │       ├── branch.md
│   │       └── ...
│   │
│   ├── ice/
│   │   ├── _manifest.yml
│   │   ├── core.md
│   │   ├── options.md
│   │   ├── models.md
│   │   └── io.md
│   │   │
│   │   ├── core/
│   │   │   ├── Model.md
│   │   │   ├── Outline.md
│   │   │   ├── Source.md
│   │   │   ├── Coach.md
│   │   │   └── Step.md
│   │   ├── options/
│   │   │   ├── TrainOptions.md
│   │   │   ├── Callback.md
│   │   │   └── ...
│   │   ├── models/
│   │   │   ├── ts/
│   │   │   ├── nlp/
│   │   │   └── rl/
│   │   └── io/
│   │       ├── save.md
│   │       └── load.md
│   │
│   └── procedure/
│       ├── _manifest.yml
│       ├── core.md
│       ├── persistence.md
│       └── contracts.md
│       │
│       ├── core/
│       │   ├── Procedure.md
│       │   ├── Step.md
│       │   ├── Artifact.md
│       │   └── WorkflowReport.md
│       ├── persistence/
│       │   ├── bind.md
│       │   ├── save.md
│       │   ├── load.md
│       │   └── compatibility.md
│       └── contracts/
│           ├── PersistContract.md
│           ├── Manifest.md
│           └── ...
│
└── generated/
    ├── api_nav/
    ├── coverage/
    └── signature_checks/
```

## Intended Left Sidebar Shape

At the top level, the sidebar should read approximately like this:

```text
API Reference
  PlussAI API Reference
    classic
    nets
    nlp
    lymph
    ice
    procedure
```

When the user opens `classic`, the sidebar should expand like this:

```text
API Reference
  PlussAI API Reference
    classic
      split
      prep
      metrics
      model
      tune
      io
      core
        FeatureBatch
        LabeledBatch
        ModelSpec
        TransformSpec
        Flow
      split
        holdout_split
        crossval_plan
        stratified_crossval_plan
        group_crossval_plan
        ts_crossval_plan
    nets
    nlp
    lymph
    ice
    procedure
```

And when the user opens an object page such as `lymph::Model`, the sidebar should
still keep them anchored under:

```text
API Reference
  PlussAI API Reference
    lymph
      core
        Model
        Workflow
        Policy
        Payload
      validate
      select
      spotcheck
      feature
      metrics
      workflow
```

That is the scalable shape: family first, then category, then object.
