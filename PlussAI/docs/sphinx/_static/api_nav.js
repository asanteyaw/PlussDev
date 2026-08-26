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
            { label: "Model", path: "api/lymph/models/Model.html" },
            {
              label: "builder",
              path: "api/lymph/models/builder.html",
              children: [
                { label: "Introduction", path: "api/lymph/models/builder/Introduction.html" },
                { label: "Builder", path: "api/lymph/models/builder/Builder.html" },
              ],
            },
          ],
        },
        {
          label: "prep",
          path: "api/lymph/prep.html",
          children: [
            { label: "Introduction", path: "api/lymph/prep.html" },
            { label: "ColumnRouter", path: "api/lymph/prep/ColumnRouter.html" },
            { label: "Stage", path: "api/lymph/prep/Stage.html" },
            { label: "StandardScaler", path: "api/lymph/prep/StandardScaler.html" },
            { label: "OneHotEncoder", path: "api/lymph/prep/OneHotEncoder.html" },
            { label: "FeatureTransformer", path: "api/lymph/prep/FeatureTransformer.html" },
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
