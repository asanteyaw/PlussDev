(() => {
  const families = [
    {
      label: "Core Utilities",
      api: "api/core.html",
      manual: "manual/core.html",
      pages: [
        { label: "Deterministic Seeds", path: "api/core/seeds.html" },
        { label: "Environment", path: "api/core/environment.html" },
        { label: "env::get", path: "api/core-env/get.html" },
        { label: "env::get_bool", path: "api/core-env/get_bool.html" },
        { label: "env::get_double", path: "api/core-env/get_double.html" },
        { label: "env::get_double_clamped", path: "api/core-env/get_double_clamped.html" },
        { label: "env::get_int", path: "api/core-env/get_int.html" },
        { label: "env::get_int_clamped", path: "api/core-env/get_int_clamped.html" },
        { label: "env::get_or", path: "api/core-env/get_or.html" },
        { label: "next_rng_seed", path: "api/core-seed/next_rng_seed.html" },
        { label: "rng_seed", path: "api/core-seed/rng_seed.html" },
        { label: "seed_or_next", path: "api/core-seed/seed_or_next.html" },
      ],
    },
    {
      label: "Loaders",
      api: "api/loaders.html",
      manual: "manual/loaders.html",
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
