(() => {
  const families = [
    {
      label: "Core Utilities",
      api: "api/core.html",
      manual: "manual/core.html",
      pages: [
        { label: "Deterministic Seeds", path: "api/core/seeds.html" },
        { label: "next_rng_seed", path: "api/core-seed/next_rng_seed.html" },
        { label: "rng_seed", path: "api/core-seed/rng_seed.html" },
        { label: "seed_or_next", path: "api/core-seed/seed_or_next.html" },
        { label: "env", path: "api/core/environment.html" },
        { label: "env::get", path: "api/core-env/get.html" },
        { label: "env::get_bool", path: "api/core-env/get_bool.html" },
        { label: "env::get_double", path: "api/core-env/get_double.html" },
        { label: "env::get_double_clamped", path: "api/core-env/get_double_clamped.html" },
        { label: "env::get_int", path: "api/core-env/get_int.html" },
        { label: "env::get_int_clamped", path: "api/core-env/get_int_clamped.html" },
        { label: "env::get_or", path: "api/core-env/get_or.html" },
        { label: "Scalar casts: the as_ family", path: "api/core/scalar-casts.html" },
        { label: "tils scalar casts", path: "api/core-scalar-cast.html" },
        { label: "as_<dtype>(Config)", path: "api/core-scalar-cast/config-form.html" },
        { label: "as_<dtype>(const Series&, Config)", path: "api/core-scalar-cast/conversion-form.html" },
        { label: "as_<dtype>(value, Config)", path: "api/core-scalar-cast/construction-form.html" },
        { label: "as_<dtype>(const Series&)", path: "api/core-scalar-cast/extraction-form.html" },
        { label: "as_dtype", path: "api/core-scalar-cast/as-dtype.html" },
        { label: "to_scalar and is_scalar_extractable_v", path: "api/core-scalar-cast/to-scalar.html" },
        { label: "Dispatch: transmit and map_axis", path: "api/core/dispatch.html" },
        { label: "transmit", path: "api/core-transmit.html" },
        { label: "map_axis", path: "api/core-map-axis.html" },
        { label: "Value Flow: flow and let", path: "api/core/value-flow.html" },
        { label: "Output: OutputEngine and its options", path: "api/core/output.html" },
        { label: "OutputEngine", path: "api/core-output-engine.html" },
        { label: "OutputEngine::to", path: "api/core-output-engine/to.html" },
        { label: "OutputEngine::sep", path: "api/core-output-engine/sep.html" },
        { label: "OutputEngine::prefix", path: "api/core-output-engine/prefix.html" },
        { label: "OutputEngine::enable_levels", path: "api/core-output-engine/enable_levels.html" },
        { label: "OutputEngine::set_options", path: "api/core-output-engine/set_options.html" },
        { label: "OutputEngine::options", path: "api/core-output-engine/options.html" },
        { label: "OutputEngine::print_defaults", path: "api/core-output-engine/print_defaults.html" },
        { label: "OutputEngine::write", path: "api/core-output-engine/write.html" },
        { label: "OutputEngine::writeln", path: "api/core-output-engine/writeln.html" },
        { label: "OutputEngine::writef", path: "api/core-output-engine/writef.html" },
        { label: "OutputEngine::writelnf", path: "api/core-output-engine/writelnf.html" },
        { label: "OutputEngine::format", path: "api/core-output-engine/format.html" },
        { label: "OutputEngine::join", path: "api/core-output-engine/join.html" },
        { label: "OutputEngine::endln", path: "api/core-output-engine/endln.html" },
        { label: "OutputEngine::flush", path: "api/core-output-engine/flush.html" },
        { label: "OutputEngine::log", path: "api/core-output-engine/log.html" },
        { label: "OutputEngine::logf", path: "api/core-output-engine/logf.html" },
        { label: "OutputEngine::trace", path: "api/core-output-engine/trace.html" },
        { label: "OutputEngine::debug", path: "api/core-output-engine/debug.html" },
        { label: "OutputEngine::info", path: "api/core-output-engine/info.html" },
        { label: "OutputEngine::warn", path: "api/core-output-engine/warn.html" },
        { label: "OutputEngine::error", path: "api/core-output-engine/error.html" },
        { label: "OutputEngine::tracef", path: "api/core-output-engine/tracef.html" },
        { label: "OutputEngine::debugf", path: "api/core-output-engine/debugf.html" },
        { label: "OutputEngine::infof", path: "api/core-output-engine/infof.html" },
        { label: "OutputEngine::warnf", path: "api/core-output-engine/warnf.html" },
        { label: "OutputEngine::errorf", path: "api/core-output-engine/errorf.html" },
        { label: "runtime_format", path: "api/core-runtime-format.html" },
        { label: "print", path: "api/core-print.html" },
        { label: "println", path: "api/core-println.html" },
        { label: "OutputLevel", path: "api/core-output-level.html" },
        { label: "OutputOptions", path: "api/core-output-options.html" },
        { label: "Print options: PrintOptions and its fields", path: "api/core/print-options.html" },
        { label: "PrintOptions", path: "api/core-print-options.html" },
        { label: "PrintOptions: joining the values of a call", path: "api/core-print-options/joining.html" },
        { label: "PrintOptions: field layout", path: "api/core-print-options/layout.html" },
        { label: "PrintOptions: number formatting", path: "api/core-print-options/numbers.html" },
        { label: "PrintOptions: humanized units", path: "api/core-print-options/humanize.html" },
        { label: "PrintOptions: booleans and optionals", path: "api/core-print-options/booleans-and-optionals.html" },
        { label: "PrintOptions: strings", path: "api/core-print-options/strings.html" },
        { label: "PrintOptions: containers, maps, and tuples", path: "api/core-print-options/containers.html" },
        { label: "PrintOptions: color", path: "api/core-print-options/color.html" },
        { label: "PrintOptions: the format-string path", path: "api/core-print-options/format-path.html" },
        { label: "PrintOptions: fluent builders", path: "api/core-print-options/builders.html" },
        { label: "PrintOptions: presets", path: "api/core-print-options/presets.html" },
        { label: "Align", path: "api/core-align.html" },
        { label: "Sign", path: "api/core-sign.html" },
        { label: "Notation", path: "api/core-notation.html" },
        { label: "Humanize", path: "api/core-humanize.html" },
        { label: "BoolStyle", path: "api/core-bool-style.html" },
        { label: "CaseXf", path: "api/core-case-xf.html" },
        { label: "PairStyle", path: "api/core-pair-style.html" },
        { label: "ColorMode", path: "api/core-color-mode.html" },
        { label: "Color", path: "api/core-color.html" },
        { label: "Style", path: "api/core-style.html" },
        { label: "SyntaxTheme", path: "api/core-syntax-theme.html" },
        { label: "Suppose", path: "api/core-suppose.html" },
        { label: "let", path: "api/core-let.html" },
      ],
    },
    {
      label: "Loaders",
      api: "api/loaders.html",
      manual: "manual/loaders.html",
      pages: [
        { label: "DataBatch", path: "api/loader-data-batch.html" },
        { label: "Dataset and SeriesDataset", path: "api/loader-dataset.html" },
        { label: "DataLoader", path: "api/loader-data-loader.html" },
        { label: "DataLoader::accessors", path: "api/loader-data-loader/accessors.html" },
        { label: "DataLoader::iteration", path: "api/loader-data-loader/iteration.html" },
        { label: "DataLoader::reset-and-epoch", path: "api/loader-data-loader/reset-and-epoch.html" },
        { label: "DataLoaderOptions", path: "api/loader-data-loader-options.html" },
        { label: "Sampler and its implementations", path: "api/loader-sampler.html" },
        { label: "split_fields and split_loaders", path: "api/loader-split.html" },
        { label: "temporal_split_indices, temporal_split_fields, and temporal_split_loaders", path: "api/loader-temporal-split.html" },
        { label: "PathContext and UrlContext", path: "api/loader-path-context.html" },
        { label: "loader path and root functions", path: "api/loader-paths.html" },
      ],
    },
    {
      label: "PlussBI",
      api: "api/bi.html",
      manual: "manual/bi.html",
    },
    {
      label: "PlussView",
      api: "api/view.html",
      manual: "manual/view.html",
    },
  ];

  function root() {
    const contentRoot = document.documentElement.getAttribute("data-content_root");
    if (typeof contentRoot === "string") {
      return contentRoot;
    }
    if (window.DOCUMENTATION_OPTIONS &&
        typeof window.DOCUMENTATION_OPTIONS.URL_ROOT === "string") {
      return window.DOCUMENTATION_OPTIONS.URL_ROOT;
    }
    return "";
  }

  function absolute(path) {
    return new URL(`${root()}${path}`, window.location.href).pathname;
  }

  function link(label, path, depth = 0) {
    const node = document.createElement("a");
    node.className = `tree-leaf depth-${depth}`;
    node.href = `${root()}${path}`;
    node.textContent = label;
    if (absolute(path) === window.location.pathname) {
      node.classList.add("active");
      node.setAttribute("aria-current", "page");
    }
    return node;
  }

  function build() {
    const path = window.location.pathname;
    const ownsPage = path.endsWith("/api_reference.html") || path.includes("/api/");
    if (!ownsPage) {
      return;
    }

    const wrapper = document.querySelector(".sphinxsidebarwrapper");
    if (!wrapper || wrapper.querySelector(".pluss-api-sidebar")) {
      return;
    }

    document.body.classList.add("apiref-page");
    const search = wrapper.querySelector("#searchbox");
    Array.from(wrapper.children).forEach((child) => {
      if (child !== search) {
        child.remove();
      }
    });

    const sidebar = document.createElement("div");
    sidebar.className = "pluss-api-sidebar";

    const title = document.createElement("div");
    title.className = "sidebar-title";
    title.textContent = "PlussTils API";
    sidebar.appendChild(title);

    const nav = document.createElement("nav");
    nav.className = "pluss-api-tree";
    nav.setAttribute("aria-label", "PlussTils API navigation");
    nav.appendChild(link("API Reference", "api_reference.html"));

    families.forEach((family) => {
      const branch = document.createElement("div");
      branch.className = "tree-branch open";

      const row = document.createElement("div");
      row.className = "tree-row";
      const toggle = document.createElement("button");
      toggle.type = "button";
      toggle.className = "tree-toggle";
      toggle.setAttribute("aria-expanded", "true");
      toggle.setAttribute("aria-label", `Collapse ${family.label}`);
      toggle.addEventListener("click", () => {
        const open = !branch.classList.contains("open");
        branch.classList.toggle("open", open);
        toggle.setAttribute("aria-expanded", open ? "true" : "false");
        toggle.setAttribute("aria-label", `${open ? "Collapse" : "Expand"} ${family.label}`);
      });
      row.appendChild(toggle);

      const familyLink = link(family.label, family.api);
      familyLink.className = "tree-link" + (familyLink.classList.contains("active") ? " active" : "");
      row.appendChild(familyLink);
      branch.appendChild(row);

      const children = document.createElement("div");
      children.className = "tree-children";
      children.appendChild(link("Manual", family.manual, 1));
      children.appendChild(link("API Reference", family.api, 1));
      (family.pages || []).forEach((page) => {
        children.appendChild(link(page.label, page.path, 1));
      });
      branch.appendChild(children);
      nav.appendChild(branch);
    });

    sidebar.appendChild(nav);
    wrapper.insertBefore(sidebar, search || wrapper.firstChild);
  }

  document.addEventListener("DOMContentLoaded", build);
})();
