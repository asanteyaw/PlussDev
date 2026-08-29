window.PLUSS_API_TREE = {
  label: "PlussAI API Reference",
  path: "api/plussai_root.html",
  children: [
    {
      label: "classic",
      path: "api/plussai_classic.html",
      children: [
        { label: "Introduction", path: "api/plussai_classic.html" },
      ],
    },
    {
      label: "nets",
      path: "api/plussai_nets.html",
      children: [
        { label: "Introduction", path: "api/plussai_nets.html" },
      ],
    },
    {
      label: "nlp",
      path: "api/plussai_nlp.html",
      children: [
        { label: "Introduction", path: "api/plussai_nlp.html" },
      ],
    },
    {
      label: "lymph",
      path: "api/plussai_lymph.html",
      children: [
        {
          label: "models",
          path: "api/lymph/models.html",
          children: [
            { label: "Introduction", path: "api/lymph/models.html" },
            {
              label: "Model",
              path: "api/lymph/models/Model.html",
              children: [
                { label: "Introduction", path: "api/lymph/models/Model/Introduction.html" },
                { label: "append", path: "api/lymph/models/Model/append.html" },
                { label: "assess", path: "api/lymph/models/Model/assess.html" },
                { label: "infer", path: "api/lymph/models/Model/infer.html" },
                { label: "learn", path: "api/lymph/models/Model/learn.html" },
                { label: "load", path: "api/lymph/models/Model/load.html" },
                { label: "prepare", path: "api/lymph/models/Model/prepare.html" },
                { label: "save", path: "api/lymph/models/Model/save.html" },
                { label: "Notes", path: "api/lymph/models/Model/Notes.html" },
              ],
            },
            {
              label: "builder",
              path: "api/lymph/models/builder.html",
              children: [
                { label: "Introduction", path: "api/lymph/models/builder/Introduction.html" },
                {
                  label: "Builder",
                  path: "api/lymph/models/builder/Builder.html",
                  children: [
                    { label: "Introduction", path: "api/lymph/models/builder/Builder/Introduction.html" },
                    { label: "benchmark", path: "api/lymph/models/builder/Builder/benchmark.html" },
                    { label: "benchmarked", path: "api/lymph/models/builder/Builder/benchmarked.html" },
                    { label: "best", path: "api/lymph/models/builder/Builder/best.html" },
                    { label: "eject", path: "api/lymph/models/builder/Builder/eject.html" },
                    { label: "get", path: "api/lymph/models/builder/Builder/get.html" },
                    { label: "has", path: "api/lymph/models/builder/Builder/has.html" },
                    { label: "inject", path: "api/lymph/models/builder/Builder/inject.html" },
                    { label: "list", path: "api/lymph/models/builder/Builder/list.html" },
                    { label: "names", path: "api/lymph/models/builder/Builder/names.html" },
                    { label: "report", path: "api/lymph/models/builder/Builder/report.html" },
                    { label: "worst", path: "api/lymph/models/builder/Builder/worst.html" },
                    { label: "Notes", path: "api/lymph/models/builder/Builder/Notes.html" },
                  ],
                },
              ],
            },
          ],
        },
        {
          label: "prep",
          path: "api/lymph/prep.html",
          children: [
            { label: "Introduction", path: "api/lymph/prep.html" },
            {
              label: "route",
              path: "api/lymph/prep/route.html",
              children: [
                { label: "Introduction", path: "api/lymph/prep/route/Introduction.html" },
                {
                  label: "Route",
                  path: "api/lymph/prep/route/Route.html",
                  children: [
                    { label: "Introduction", path: "api/lymph/prep/route/Route/Introduction.html" },
                    { label: "resolve", path: "api/lymph/prep/route/Route/resolve.html" },
                    { label: "with_remainder", path: "api/lymph/prep/route/Route/with_remainder.html" },
                    { label: "Notes", path: "api/lymph/prep/route/Route/Notes.html" },
                  ],
                },
                { label: "names", path: "api/lymph/prep/route/names.html" },
                { label: "remaining_of", path: "api/lymph/prep/route/remaining_of.html" },
                { label: "where", path: "api/lymph/prep/route/where.html" },
              ],
            },
            {
              label: "stage",
              path: "api/lymph/prep/stage_family.html",
              children: [
                { label: "Introduction", path: "api/lymph/prep/stage/Introduction.html" },
                {
                  label: "Stage",
                  path: "api/lymph/prep/stage/Stage.html",
                  children: [
                    { label: "Introduction", path: "api/lymph/prep/stage/Stage/Introduction.html" },
                    { label: "contract", path: "api/lymph/prep/stage/Stage/contract.html" },
                    { label: "Notes", path: "api/lymph/prep/stage/Stage/Notes.html" },
                  ],
                },
                { label: "one_hot_encoder", path: "api/lymph/prep/stage/one_hot_encoder.html" },
                { label: "standard_scaler", path: "api/lymph/prep/stage/standard_scaler.html" },
              ],
            },
            {
              label: "scaling",
              path: "api/lymph/prep/scaling.html",
              children: [
                { label: "Introduction", path: "api/lymph/prep/scaling/Introduction.html" },
                {
                  label: "StandardScaler",
                  path: "api/lymph/prep/scaling/StandardScaler.html",
                  children: [
                    { label: "Introduction", path: "api/lymph/prep/scaling/StandardScaler/Introduction.html" },
                    { label: "features", path: "api/lymph/prep/scaling/StandardScaler/features.html" },
                    { label: "learn", path: "api/lymph/prep/scaling/StandardScaler/learn.html" },
                    { label: "learn_map", path: "api/lymph/prep/scaling/StandardScaler/learn_map.html" },
                    { label: "learned", path: "api/lymph/prep/scaling/StandardScaler/learned.html" },
                    { label: "map", path: "api/lymph/prep/scaling/StandardScaler/map.html" },
                    { label: "summary", path: "api/lymph/prep/scaling/StandardScaler/summary.html" },
                    { label: "Notes", path: "api/lymph/prep/scaling/StandardScaler/Notes.html" },
                  ],
                },
                { label: "StandardScalerOptions", path: "api/lymph/prep/scaling/StandardScalerOptions.html" },
              ],
            },
            {
              label: "encoding",
              path: "api/lymph/prep/encoding.html",
              children: [
                { label: "Introduction", path: "api/lymph/prep/encoding/Introduction.html" },
                {
                  label: "OneHotEncoder",
                  path: "api/lymph/prep/encoding/OneHotEncoder.html",
                  children: [
                    { label: "Introduction", path: "api/lymph/prep/encoding/OneHotEncoder/Introduction.html" },
                    { label: "features", path: "api/lymph/prep/encoding/OneHotEncoder/features.html" },
                    { label: "learn", path: "api/lymph/prep/encoding/OneHotEncoder/learn.html" },
                    { label: "learn_map", path: "api/lymph/prep/encoding/OneHotEncoder/learn_map.html" },
                    { label: "learned", path: "api/lymph/prep/encoding/OneHotEncoder/learned.html" },
                    { label: "map", path: "api/lymph/prep/encoding/OneHotEncoder/map.html" },
                    { label: "summary", path: "api/lymph/prep/encoding/OneHotEncoder/summary.html" },
                    { label: "Notes", path: "api/lymph/prep/encoding/OneHotEncoder/Notes.html" },
                  ],
                },
                { label: "OneHotEncoderOptions", path: "api/lymph/prep/encoding/OneHotEncoderOptions.html" },
              ],
            },
            {
              label: "feature",
              path: "api/lymph/prep/feature.html",
              children: [
                { label: "Introduction", path: "api/lymph/prep/feature/Introduction.html" },
                {
                  label: "FeatureTransformer",
                  path: "api/lymph/prep/feature/FeatureTransformer.html",
                  children: [
                    { label: "Introduction", path: "api/lymph/prep/feature/FeatureTransformer/Introduction.html" },
                    { label: "learn", path: "api/lymph/prep/feature/FeatureTransformer/learn.html" },
                    { label: "learn_map", path: "api/lymph/prep/feature/FeatureTransformer/learn_map.html" },
                    { label: "map", path: "api/lymph/prep/feature/FeatureTransformer/map.html" },
                    { label: "stage_count", path: "api/lymph/prep/feature/FeatureTransformer/stage_count.html" },
                    { label: "trained", path: "api/lymph/prep/feature/FeatureTransformer/trained.html" },
                    { label: "Notes", path: "api/lymph/prep/feature/FeatureTransformer/Notes.html" },
                  ],
                },
                { label: "feature_transformer", path: "api/lymph/prep/feature/feature_transformer.html" },
              ],
            },
          ],
        },
        {
          label: "workflow",
          path: "api/lymph/model.html",
          children: [
            { label: "Introduction", path: "api/lymph/model.html" },
            { label: "Workflow", path: "api/lymph/model/Workflow.html" },
            { label: "Payload", path: "api/lymph/model/Payload.html" },
            { label: "Policy", path: "api/lymph/model/Policy.html" },
          ],
        },
        {
          label: "selection",
          path: "api/lymph/tune.html",
          children: [
            {
              label: "validate",
              path: "api/lymph/split.html",
              children: [
                { label: "Introduction", path: "api/lymph/split.html" },
                { label: "Plan", path: "api/lymph/split/Plan.html" },
                { label: "ValidationOptions", path: "api/lymph/split/ValidationOptions.html" },
                { label: "cross", path: "api/lymph/split/cross.html" },
                { label: "holdout", path: "api/lymph/split/holdout.html" },
                { label: "time", path: "api/lymph/split/time.html" },
              ],
            },
            {
              label: "search",
              path: "api/lymph/tune/Search.html",
              children: [
                { label: "Introduction", path: "api/lymph/tune.html" },
                { label: "Search", path: "api/lymph/tune/Search.html" },
                { label: "CompareReport", path: "api/lymph/tune/CompareReport.html" },
                { label: "TuneReport", path: "api/lymph/tune/TuneReport.html" },
              ],
            },
            {
              label: "spotcheck",
              path: "api/lymph/tune/Suite.html",
              children: [
                { label: "Suite", path: "api/lymph/tune/Suite.html" },
              ],
            },
          ],
        },
        {
          label: "metrics",
          path: "api/lymph/metrics.html",
          children: [
            { label: "Introduction", path: "api/lymph/metrics.html" },
            { label: "CrossValidationReport", path: "api/lymph/metrics/CrossValidationReport.html" },
            { label: "Threshold", path: "api/lymph/metrics/Threshold.html" },
            { label: "classification_summary", path: "api/lymph/metrics/classification_summary.html" },
            { label: "confusion_table", path: "api/lymph/metrics/confusion_table.html" },
          ],
        },
        {
          label: "io",
          path: "api/lymph/io.html",
          children: [
            { label: "Introduction", path: "api/lymph/io.html" },
            { label: "load", path: "api/lymph/io/load.html" },
            { label: "persistence", path: "api/lymph/io/persistence.html" },
            { label: "save", path: "api/lymph/io/save.html" },
          ],
        },
      ],
    },
    {
      label: "ice",
      path: "api/plussai_ice.html",
      children: [
        { label: "Introduction", path: "api/plussai_ice.html" },
      ],
    },
    {
      label: "procedure",
      path: "api/plussai_procedure.html",
      children: [
        { label: "Introduction", path: "api/plussai_procedure.html" },
      ],
    },
  ],
};
