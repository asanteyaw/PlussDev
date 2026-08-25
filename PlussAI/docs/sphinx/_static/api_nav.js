window.PLUSS_API_TREE = {
  label: "PlussAI API Reference",
  path: "api/plussai_root.html",
  children: [
    {
      label: "classic",
      path: "api/plussai_classic.html",
      children: [],
    },
    {
      label: "nets",
      path: "api/plussai_nets.html",
      children: [],
    },
    {
      label: "nlp",
      path: "api/plussai_nlp.html",
      children: [],
    },
    {
      label: "lymph",
      path: "api/plussai_lymph.html",
      children: [
        {
          label: "split",
          path: "api/lymph/split.html",
          children: [
            { label: "ValidationOptions", path: "api/lymph/split/ValidationOptions.html" },
            { label: "Plan", path: "api/lymph/split/Plan.html" },
            { label: "cross", path: "api/lymph/split/cross.html" },
            { label: "time", path: "api/lymph/split/time.html" },
            { label: "holdout", path: "api/lymph/split/holdout.html" },
          ],
        },
        {
          label: "prep",
          path: "api/lymph/prep.html",
          children: [
            { label: "ColumnRouter", path: "api/lymph/prep/ColumnRouter.html" },
            { label: "Stage", path: "api/lymph/prep/Stage.html" },
            { label: "StandardScaler", path: "api/lymph/prep/StandardScaler.html" },
            { label: "OneHotEncoder", path: "api/lymph/prep/OneHotEncoder.html" },
            { label: "FeatureTransformer", path: "api/lymph/prep/FeatureTransformer.html" },
          ],
        },
        {
          label: "metrics",
          path: "api/lymph/metrics.html",
          children: [
            { label: "Threshold", path: "api/lymph/metrics/Threshold.html" },
            { label: "CrossValidationReport", path: "api/lymph/metrics/CrossValidationReport.html" },
            { label: "confusion_table", path: "api/lymph/metrics/confusion_table.html" },
            { label: "classification_summary", path: "api/lymph/metrics/classification_summary.html" },
          ],
        },
        {
          label: "model",
          path: "api/lymph/model.html",
          children: [
            { label: "Model", path: "api/lymph/model/Model.html" },
            { label: "Workflow", path: "api/lymph/model/Workflow.html" },
            { label: "Builder", path: "api/lymph/model/Builder.html" },
            { label: "Policy", path: "api/lymph/model/Policy.html" },
            { label: "Payload", path: "api/lymph/model/Payload.html" },
          ],
        },
        {
          label: "tune",
          path: "api/lymph/tune.html",
          children: [
            { label: "Suite", path: "api/lymph/tune/Suite.html" },
            { label: "Search", path: "api/lymph/tune/Search.html" },
            { label: "CompareReport", path: "api/lymph/tune/CompareReport.html" },
            { label: "TuneReport", path: "api/lymph/tune/TuneReport.html" },
          ],
        },
        {
          label: "io",
          path: "api/lymph/io.html",
          children: [
            { label: "save", path: "api/lymph/io/save.html" },
            { label: "load", path: "api/lymph/io/load.html" },
            { label: "persistence", path: "api/lymph/io/persistence.html" },
          ],
        },
      ],
    },
    {
      label: "ice",
      path: "api/plussai_ice.html",
      children: [],
    },
    {
      label: "procedure",
      path: "api/plussai_procedure.html",
      children: [],
    },
  ],
};
