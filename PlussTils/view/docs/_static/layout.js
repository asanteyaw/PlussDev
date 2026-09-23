(() => {
  function text(node) {
    return ((node && node.textContent) || "").trim();
  }

  function cleanHeadingText(raw) {
    return (raw || "").replace(/\u00B6/g, "").trim();
  }

  function extractOverloadLabel(section, title) {
    const sigBlock = section.querySelector(':scope > section > .highlight pre');
    const sigText = cleanHeadingText(text(sigBlock));
    if (!sigText) {
      return title;
    }

    const match = sigText.match(/\b([A-Za-z_][A-Za-z0-9_]*)\s*\((.*)\)$/);
    if (!match) {
      return title;
    }

    const rawArgs = (match[2] || "").trim();
    if (!rawArgs || rawArgs === "self") {
      return `${title}()`;
    }

    const args = rawArgs
      .split(",")
      .map((part) => part.trim())
      .filter((part) => part && part !== "self");

    if (!args.length) {
      return `${title}()`;
    }

    return `${title}(${args.join(", ")})`;
  }

  // Recovers the site's project name from the page <title>, which Sphinx
  // renders as "<page title> — <project>  documentation" (or just
  // "<project>  documentation" on the index page itself). This keeps the
  // topbar brand correct per-site without hardcoding any one package's name.
  function siteBrandFromTitle() {
    const raw = document.title || "";
    const parts = raw.split("—").map((p) => p.trim()).filter(Boolean);
    let candidate = parts.length ? parts[parts.length - 1] : raw.trim();
    candidate = candidate.replace(/\s*documentation\s*$/i, "").trim();
    candidate = candidate.replace(/\s{2,}/g, " ");
    return candidate || "Pluss";
  }

  // Sphinx exposes the relative path back to this site's root as
  // DOCUMENTATION_OPTIONS.URL_ROOT (e.g. "../../" from a nested API page).
  // Using it means topbar links resolve correctly no matter how deep the
  // current page is nested.
  function urlRoot() {
    if (window.DOCUMENTATION_OPTIONS && typeof window.DOCUMENTATION_OPTIONS.URL_ROOT === "string") {
      return window.DOCUMENTATION_OPTIONS.URL_ROOT;
    }
    const htmlRoot = document.documentElement.getAttribute("data-content_root");
    if (typeof htmlRoot === "string" && htmlRoot.length) {
      return htmlRoot;
    }
    return "";
  }

  function buildTopbar() {
    const root = urlRoot();
    const currentPath = window.location.pathname.split("/").pop() || "index.html";

    const topbar = document.createElement("div");
    topbar.id = "pluss-topbar";

    const brand = document.createElement("a");
    brand.className = "brand";
    brand.href = `${root}index.html`;
    brand.textContent = siteBrandFromTitle();
    topbar.appendChild(brand);

    const links = document.createElement("div");
    links.className = "top-links";
    topbar.appendChild(links);

    // Deliberately small and fixed: this site's own in-page sidebar already
    // carries the detailed navigation (method list, section list, etc.), so
    // the topbar only needs to get you Home or into the API reference.
    const navItems = [
      { href: `${root}index.html`, label: "Home", match: "index.html" },
      { href: `${root}api_reference.html`, label: "API Reference", match: "api_reference.html" },
    ];

    navItems.forEach(({ href, label, match }) => {
      const a = document.createElement("a");
      a.href = href;
      a.textContent = label;
      if (currentPath === match) {
        a.classList.add("active");
      }
      links.appendChild(a);
    });

    document.body.insertBefore(topbar, document.body.firstChild);
  }

  function isPlussApiPage() {
    if (!window.PLUSS_API_TREE) {
      return false;
    }
    return /\/api\/(?:plussai_[^/]+|lymph\/.+|lymph\.html)$/.test(window.location.pathname);
  }

  // True on any page api_nav.js already owns (sets <body class="apiref-page">
  // and builds its own .apiref-nav sidebar tree, independently of this
  // file's buildApiTreeSidebar()/buildMethodSidebar()/buildPrevNext() path,
  // which predates api_nav.js and is otherwise dead: window.PLUSS_API_TREE
  // is never assigned anymore, so isPlussApiPage() above always returns
  // false and buildApiTreeSidebar() itself would crash (reads
  // PLUSS_API_TREE.children) if that early return were ever removed. Do not
  // reactivate that path -- gate the *legacy* method-sidebar/prev-next
  // fallback further down on this instead, so it never fires on a page
  // api_nav.js already fully navigates.
  function hasModernApiNav() {
    return document.body.classList.contains("apiref-page");
  }

  function isPlussAiApiPage() {
    return siteBrandFromTitle() === "PlussAI" && /\/api\//.test(window.location.pathname);
  }

  function docBaseUrl() {
    return new URL(urlRoot(), window.location.href);
  }

  function absoluteDocPath(path) {
    if (!path) {
      return "";
    }
    try {
      return new URL(path, docBaseUrl()).pathname;
    } catch (_err) {
      return "";
    }
  }

  function nodeContainsCurrentPage(node) {
    if (!node) {
      return false;
    }
    const current = window.location.pathname;
    if (node.path && absoluteDocPath(node.path) === current) {
      return true;
    }
    return Array.isArray(node.children) && node.children.some((child) => nodeContainsCurrentPage(child));
  }

  function buildApiTreeSidebar() {
    if (!isPlussApiPage()) {
      return false;
    }

    if (document.querySelector(".pluss-api-sidebar")) {
      return true;
    }

    const wrapper = document.querySelector(".sphinxsidebarwrapper");
    if (!wrapper) {
      return false;
    }

    const searchBox = wrapper.querySelector("#searchbox");
    Array.from(wrapper.children).forEach((child) => {
      if (child !== searchBox) {
        child.remove();
      }
    });

    const sidebar = document.createElement("div");
    sidebar.className = "pluss-api-sidebar";

    const title = document.createElement("div");
    title.className = "sidebar-title";
    title.textContent = "API Reference";
    sidebar.appendChild(title);

    const nav = document.createElement("nav");
    nav.className = "pluss-api-tree";
    nav.setAttribute("aria-label", "Pluss API navigation");
    sidebar.appendChild(nav);

    const makeLink = (node) => {
      const link = document.createElement("a");
      link.textContent = node.label || "";
      link.href = node.path ? new URL(node.path, docBaseUrl()).href : "#";
      if (node.path && absoluteDocPath(node.path) === window.location.pathname) {
        link.classList.add("active");
        link.setAttribute("aria-current", "page");
      }
      return link;
    };

    const renderNode = (node, depth) => {
      const hasChildren = Array.isArray(node.children) && node.children.length > 0;
      if (hasChildren) {
        const branch = document.createElement("div");
        branch.className = `tree-branch depth-${depth}`;
        if (nodeContainsCurrentPage(node)) {
          branch.classList.add("open");
        }

        const row = document.createElement("div");
        row.className = "tree-row";

        const toggle = document.createElement("button");
        toggle.type = "button";
        toggle.className = "tree-toggle";
        toggle.setAttribute("aria-label", `${branch.classList.contains("open") ? "Collapse" : "Expand"} ${node.label || "section"}`);
        toggle.setAttribute("aria-expanded", branch.classList.contains("open") ? "true" : "false");
        toggle.addEventListener("click", () => {
          const opening = !branch.classList.contains("open");
          branch.classList.toggle("open", opening);
          toggle.setAttribute("aria-expanded", opening ? "true" : "false");
          toggle.setAttribute("aria-label", `${opening ? "Collapse" : "Expand"} ${node.label || "section"}`);
        });
        row.appendChild(toggle);

        const link = makeLink(node);
        link.classList.add("tree-link");
        row.appendChild(link);
        branch.appendChild(row);

        const children = document.createElement("div");
        children.className = "tree-children";
        node.children.forEach((child) => {
          children.appendChild(renderNode(child, depth + 1));
        });
        branch.appendChild(children);
        return branch;
      }

      const leaf = makeLink(node);
      leaf.classList.add("tree-leaf", `depth-${depth}`);
      return leaf;
    };

    const treeRoot = window.PLUSS_API_TREE;
    const topNodes = Array.isArray(treeRoot.children) ? treeRoot.children : [];
    topNodes.forEach((child) => {
      nav.appendChild(renderNode(child, 0));
    });

    wrapper.insertBefore(sidebar, searchBox || wrapper.firstChild);
    return true;
  }

  function buildMethodSidebar() {
    if (isPlussAiApiPage() || window.PLUSS_API_TREE || document.querySelector(".pluss-api-sidebar")) {
      return null;
    }
    const apiRoots = Array.from(document.querySelectorAll("section")).filter((section) => {
      const h2 = section.querySelector(":scope > h2");
      return h2 && text(h2).startsWith("API Reference Style");
    });

    if (!apiRoots.length) {
      return;
    }

    const bodywrapper = document.querySelector(".bodywrapper");
    const documentRoot = document.querySelector("div.document");
    if (!bodywrapper || !documentRoot) {
      return;
    }

    const methodItems = [];
    apiRoots.forEach((root) => {
      const groupTitleNode = root.parentElement
        ? root.parentElement.querySelector(":scope > h1, :scope > h2")
        : null;
      const fallbackGroup = cleanHeadingText(text(groupTitleNode)) || "Methods";

      const container = root.parentElement || root;
      const isAfterRoot = (node) => {
        if (node === root) {
          return false;
        }
        if (root.contains(node)) {
          return true;
        }
        const rel = root.compareDocumentPosition(node);
        return Boolean(rel & Node.DOCUMENT_POSITION_FOLLOWING);
      };

      const methodSections = Array.from(container.querySelectorAll("section")).filter((node) => {
        return node.querySelector(":scope > h3") && isAfterRoot(node);
      });

      const groupForSection = (section) => {
        let current = section.parentElement;
        while (current && current !== root) {
          if (current.tagName === "SECTION") {
            const h2 = current.querySelector(":scope > h2");
            if (h2) {
              return cleanHeadingText(text(h2)) || fallbackGroup;
            }
          }
          current = current.parentElement;
        }
        return fallbackGroup;
      };

      methodSections.forEach((section, index) => {
        if (!section.id) {
          section.id = `method-${fallbackGroup.toLowerCase().replace(/\W+/g, "-")}-${index + 1}`;
        }
        const h3 = section.querySelector(":scope > h3");
        const codeTitle = h3 ? h3.querySelector("code") : null;
        const title = cleanHeadingText(text(codeTitle) || text(h3));
        const overloadLabel = extractOverloadLabel(section, title || "method");
        methodItems.push({
          root,
          section,
          id: section.id,
          title: title || "method",
          overloadLabel,
          group: groupForSection(section),
        });
      });
    });

    if (!methodItems.length) {
      return null;
    }

    methodItems.sort((a, b) => {
      const ta = (a.title || "").toLowerCase();
      const tb = (b.title || "").toLowerCase();
      if (ta !== tb) {
        return ta.localeCompare(tb);
      }
      const oa = (a.overloadLabel || "").toLowerCase();
      const ob = (b.overloadLabel || "").toLowerCase();
      if (oa !== ob) {
        return oa.localeCompare(ob);
      }
      return (a.id || "").localeCompare(b.id || "");
    });

    document.body.classList.add("has-method-sidebar");

    // Hide everything before the API root inside each container so API starts immediately.
    apiRoots.forEach((root) => {
      const parent = root.parentElement;
      if (!parent) {
        return;
      }
      let reachedRoot = false;
      Array.from(parent.children).forEach((child) => {
        if (child === root) {
          reachedRoot = true;
          return;
        }
        if (!reachedRoot) {
          child.classList.add("pluss-pre-api-hidden");
        }
      });

      // Hide API section chrome before first method (title/intro text) so method appears first.
      let reachedMethod = false;
      Array.from(root.children).forEach((child) => {
        if (child.tagName === "SECTION" && child.querySelector("h3")) {
          reachedMethod = true;
          return;
        }
        if (!reachedMethod) {
          child.classList.add("pluss-api-intro-hidden");
        }
      });
    });

    const dupCount = new Map();
    methodItems.forEach((m) => {
      const key = `${m.group}::${m.title}`;
      dupCount.set(key, (dupCount.get(key) || 0) + 1);
    });
    const seen = new Map();

    const sidebar = document.createElement("aside");
    sidebar.id = "pluss-method-sidebar";

    const title = document.createElement("div");
    title.className = "sidebar-title";
    title.textContent = "Method Navigation";
    sidebar.appendChild(title);

    const nav = document.createElement("nav");
    nav.className = "method-nav";
    sidebar.appendChild(nav);

    let currentGroup = "";
    const links = [];
    methodItems.forEach((m, idx) => {
      if (m.group !== currentGroup) {
        currentGroup = m.group;
        const groupHeader = document.createElement("div");
        groupHeader.className = "method-group";
        groupHeader.textContent = currentGroup;
        nav.appendChild(groupHeader);
      }

      const key = `${m.group}::${m.title}`;
      const n = (seen.get(key) || 0) + 1;
      seen.set(key, n);
      const label = dupCount.get(key) > 1 ? m.overloadLabel : m.title;

      const a = document.createElement("a");
      a.href = `#${m.id}`;
      a.textContent = label;
      a.dataset.methodIndex = String(idx);
      nav.appendChild(a);
      links.push(a);
    });

    documentRoot.appendChild(sidebar);

    let activeIndex = 0;
    const listeners = [];
    const setActive = (idx, opts = {}) => {
      const { updateHash = true, resetScroll = true } = opts;
      if (idx < 0 || idx >= methodItems.length) {
        return;
      }
      activeIndex = idx;
      const active = methodItems[idx];

      apiRoots.forEach((root) => {
        root.classList.toggle("pluss-api-root-hidden", root !== active.root);
      });

      methodItems.forEach((item, i) => {
        item.section.classList.toggle("pluss-method-hidden", i !== idx);
      });

      links.forEach((a, i) => {
        a.classList.toggle("active", i === idx);
      });

      if (updateHash) {
        history.replaceState(null, "", `#${active.id}`);
      }
      if (resetScroll) {
        bodywrapper.scrollTop = 0;
      }

      listeners.forEach((fn) => fn(activeIndex));
    };

    links.forEach((a) => {
      a.addEventListener("click", (ev) => {
        ev.preventDefault();
        const idx = Number(a.dataset.methodIndex || "0");
        setActive(idx, { updateHash: true, resetScroll: true });
      });
    });

    const fromHash = (() => {
      const hash = window.location.hash.replace(/^#/, "");
      if (!hash) {
        return -1;
      }
      return methodItems.findIndex((m) => m.id === hash);
    })();
    setActive(fromHash >= 0 ? fromHash : 0, { updateHash: fromHash < 0, resetScroll: false });

    window.addEventListener("hashchange", () => {
      const hash = window.location.hash.replace(/^#/, "");
      const idx = methodItems.findIndex((m) => m.id === hash);
      if (idx >= 0 && idx !== activeIndex) {
        setActive(idx, { updateHash: false, resetScroll: true });
      }
    });

    return {
      items: methodItems,
      getActiveIndex: () => activeIndex,
      setActive: (idx) => setActive(idx, { updateHash: true, resetScroll: true }),
      onChange: (fn) => {
        if (typeof fn === "function") {
          listeners.push(fn);
        }
      },
    };
  }

  function buildSectionSidebar() {
    if (isPlussAiApiPage() || window.PLUSS_API_TREE || document.querySelector(".pluss-api-sidebar")) {
      return null;
    }
    const documentRoot = document.querySelector("div.document");
    const bodywrapper = document.querySelector(".bodywrapper");
    if (!documentRoot || !bodywrapper) {
      return null;
    }

    const pageRoots = Array.from(document.querySelectorAll("div.body > section"));
    const pageRoot = pageRoots.find((root) => {
      return Array.from(root.children).some((node) => {
        return node.tagName === "SECTION" && node.querySelector(":scope > h2");
      });
    });
    if (!pageRoot) {
      return null;
    }

    const sections = Array.from(pageRoot.children).filter((node) => node.tagName === "SECTION" && node.querySelector(":scope > h2"));
    if (!sections.length) {
      return null;
    }

    document.body.classList.add("has-method-sidebar");

    const sidebar = document.createElement("aside");
    sidebar.id = "pluss-method-sidebar";

    const title = document.createElement("div");
    title.className = "sidebar-title";
    title.textContent = "Section Navigation";
    sidebar.appendChild(title);

    const nav = document.createElement("nav");
    nav.className = "method-nav";
    sidebar.appendChild(nav);

    const items = [];
    sections.forEach((section, idx) => {
      if (!section.id) {
        section.id = `section-${idx + 1}`;
      }
      const h2 = section.querySelector(":scope > h2");
      const item = {
        id: section.id,
        title: cleanHeadingText(text(h2)),
        section,
      };
      items.push(item);
    });

    const links = [];
    items.forEach((item, idx) => {
      const a = document.createElement("a");
      a.href = `#${item.id}`;
      a.textContent = item.title;
      a.dataset.sectionIndex = String(idx);
      nav.appendChild(a);
      links.push(a);
    });

    documentRoot.appendChild(sidebar);

    let activeIndex = 0;
    const setActive = (idx, opts = {}) => {
      const { updateHash = true, resetScroll = true } = opts;
      if (idx < 0 || idx >= items.length) {
        return;
      }
      activeIndex = idx;
      items.forEach((item, i) => {
        item.section.classList.toggle("pluss-method-hidden", i !== idx);
      });
      links.forEach((a, i) => {
        a.classList.toggle("active", i === idx);
      });
      if (updateHash) {
        history.replaceState(null, "", `#${items[idx].id}`);
      }
      if (resetScroll) {
        bodywrapper.scrollTop = 0;
      }
    };

    links.forEach((a) => {
      a.addEventListener("click", (ev) => {
        ev.preventDefault();
        setActive(Number(a.dataset.sectionIndex || "0"), { updateHash: true, resetScroll: true });
      });
    });

    const fromHash = (() => {
      const hash = window.location.hash.replace(/^#/, "");
      return items.findIndex((item) => item.id === hash);
    })();
    setActive(fromHash >= 0 ? fromHash : 0, { updateHash: fromHash < 0, resetScroll: false });

    return {
      items,
      getActiveIndex: () => activeIndex,
      setActive: (idx) => setActive(idx, { updateHash: true, resetScroll: true }),
    };
  }

  function addCopyButtons() {
    document.querySelectorAll("section").forEach((section) => {
      const heading = section.querySelector(":scope > h3, :scope > h4");
      if (!heading || !/^Example\\b/i.test(heading.textContent.trim())) {
        return;
      }
      section.querySelectorAll(":scope div.highlight").forEach((block) => {
        if (block.querySelector(".pluss-copy-code")) {
          return;
        }
        const button = document.createElement("button");
        button.className = "pluss-copy-code";
        button.type = "button";
        button.title = "Copy code";
        button.setAttribute("aria-label", "Copy code");
        button.addEventListener("click", async () => {
          const pre = block.querySelector("pre");
          if (!pre) {
            return;
          }
          const value = pre.textContent;
          if (navigator.clipboard && window.isSecureContext) {
            await navigator.clipboard.writeText(value);
          } else {
            const fallback = document.createElement("textarea");
            fallback.value = value;
            fallback.setAttribute("readonly", "");
            fallback.style.cssText = "position:fixed;opacity:0";
            document.body.appendChild(fallback);
            fallback.select();
            document.execCommand("copy");
            fallback.remove();
          }
          button.classList.add("copied");
          button.title = "Copied";
          window.setTimeout(() => {
            button.classList.remove("copied");
            button.title = "Copy code";
          }, 1200);
        });
        block.appendChild(button);
      });
    });
  }

  function scheduleCopyButtons() {
    [0, 150, 600, 1800].forEach((delay) => {
      window.setTimeout(addCopyButtons, delay);
    });
  }

  function initPlussApiSidebar() {
    const tree = document.querySelector(".pluss-api-sidebar .pluss-api-tree");
    if (!tree) {
      return;
    }

    const current = window.location.pathname;
    const branchNodes = Array.from(tree.querySelectorAll(".tree-branch"));

    branchNodes.forEach((branch) => {
      const branchLink = branch.querySelector(":scope > .tree-row .tree-link");
      const currentBranch = branchLink && absoluteDocPath(branchLink.getAttribute("href")) === current;
      const activeDescendant = branch.querySelector(':scope > .tree-children a.active');
      if (currentBranch || activeDescendant) {
        branch.classList.add("open");
        const toggle = branch.querySelector(":scope > .tree-row .tree-toggle");
        if (toggle) {
          toggle.setAttribute("aria-expanded", "true");
          toggle.setAttribute("aria-label", `Collapse ${text(branchLink) || "section"}`);
        }
      }
    });
  }

  function buildPrevNext(methodNav) {
    // A page apiref.py already hand-wires (every classic/nn/... page under
    // the modern api_nav.js tree) ships its own static
    // .pluss-inline-prev-next block in the rendered HTML -- present from
    // first paint, no dependency on this script's or api_nav.js's relative
    // DOMContentLoaded timing. Never build the legacy JS-injected pager
    // (#pluss-prev-next) on top of it; that produced a redundant second
    // prev/next bar on every such page.
    if (document.querySelector(".pluss-inline-prev-next")) {
      return;
    }
    if (methodNav && methodNav.items && methodNav.items.length) {
      const bodywrapper = document.querySelector(".bodywrapper");
      if (!bodywrapper) {
        return;
      }

      const wrap = document.createElement("div");
      wrap.id = "pluss-prev-next";

      const prevLink = document.createElement("a");
      prevLink.className = "prev-link";
      prevLink.href = "#";
      const prevLabel = document.createElement("span");
      prevLabel.className = "label";
      prevLabel.textContent = "Previous";
      const prevTitle = document.createElement("span");
      prevTitle.className = "title";
      prevLink.appendChild(prevLabel);
      prevLink.appendChild(prevTitle);

      const nextLink = document.createElement("a");
      nextLink.className = "next-link";
      nextLink.href = "#";
      const nextLabel = document.createElement("span");
      nextLabel.className = "label";
      nextLabel.textContent = "Next";
      const nextTitle = document.createElement("span");
      nextTitle.className = "title";
      nextLink.appendChild(nextLabel);
      nextLink.appendChild(nextTitle);

      wrap.appendChild(prevLink);
      wrap.appendChild(nextLink);
      bodywrapper.appendChild(wrap);
      wrap.classList.add("show");

      const render = () => {
        const idx = methodNav.getActiveIndex();
        const prevIdx = idx - 1;
        const nextIdx = idx + 1;

        if (prevIdx >= 0) {
          prevLink.style.display = "";
          prevTitle.textContent = methodNav.items[prevIdx].title;
          prevLink.href = `#${methodNav.items[prevIdx].id}`;
        } else {
          prevLink.style.display = "none";
        }

        if (nextIdx < methodNav.items.length) {
          nextLink.style.display = "";
          nextTitle.textContent = methodNav.items[nextIdx].title;
          nextLink.href = `#${methodNav.items[nextIdx].id}`;
        } else {
          nextLink.style.display = "none";
        }
      };

      prevLink.addEventListener("click", (ev) => {
        ev.preventDefault();
        const idx = methodNav.getActiveIndex();
        if (idx > 0) {
          methodNav.setActive(idx - 1);
        }
      });

      nextLink.addEventListener("click", (ev) => {
        ev.preventDefault();
        const idx = methodNav.getActiveIndex();
        if (idx < methodNav.items.length - 1) {
          methodNav.setActive(idx + 1);
        }
      });

      methodNav.onChange(render);
      render();
      return;
    }

    const prev = document.querySelector('link[rel="prev"]');
    const next = document.querySelector('link[rel="next"]');
    if (!prev && !next) {
      return;
    }

    const bodywrapper = document.querySelector(".bodywrapper");
    if (!bodywrapper) {
      return;
    }

    const wrap = document.createElement("div");
    wrap.id = "pluss-prev-next";

      const makeLink = (kind, relNode) => {
        if (!relNode) {
          return null;
        }
        const a = document.createElement("a");
        a.href = relNode.getAttribute("href");
        a.className = kind.toLowerCase() + "-link";

      const label = document.createElement("span");
      label.className = "label";
      label.textContent = kind;

      const title = document.createElement("span");
      title.className = "title";
      title.textContent = relNode.getAttribute("title") || relNode.getAttribute("href");

      a.appendChild(label);
      a.appendChild(title);
      return a;
    };

    const prevLink = makeLink("Previous", prev);
    const nextLink = makeLink("Next", next);
    if (prevLink) {
      wrap.appendChild(prevLink);
    }
    if (nextLink) {
      wrap.appendChild(nextLink);
    }

    bodywrapper.appendChild(wrap);

    wrap.classList.add("show");
  }

  document.addEventListener("DOMContentLoaded", () => {
    buildTopbar();
    const hasApiTree = buildApiTreeSidebar() || hasModernApiNav();
    const methodNav = hasApiTree ? null : buildMethodSidebar();
    const sectionNav = hasApiTree ? null : (methodNav ? null : buildSectionSidebar());

    // Safety fallback: never leave the left rail empty.
    if (document.body.classList.contains("has-method-sidebar")) {
      const navCount = document.querySelectorAll("#pluss-method-sidebar .method-nav a").length;
      if (navCount === 0) {
        const sidebar = document.getElementById("pluss-method-sidebar");
        if (sidebar) {
          sidebar.remove();
        }
        document.body.classList.remove("has-method-sidebar");
      }
    }

    if (isPlussAiApiPage() && document.querySelector(".pluss-api-sidebar")) {
      const sidebar = document.getElementById("pluss-method-sidebar");
      if (sidebar) {
        sidebar.remove();
      }
      document.body.classList.remove("has-method-sidebar");
    }

    initPlussApiSidebar();
    buildPrevNext(methodNav || sectionNav);
    scheduleCopyButtons();
  });

  window.addEventListener("pageshow", () => {
    scheduleCopyButtons();
  });

  window.setTimeout(addCopyButtons, 2500);
})();
