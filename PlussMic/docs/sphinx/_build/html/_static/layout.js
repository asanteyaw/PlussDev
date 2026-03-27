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

  function buildTopbar() {
    const sidebarLinks = Array.from(
      document.querySelectorAll(".sphinxsidebar .toctree-l1 > a.reference.internal")
    );
    const unique = new Map();

    const currentPath = window.location.pathname.split("/").pop() || "index.html";

    sidebarLinks.forEach((link) => {
      let href = (link.getAttribute("href") || "").trim();
      const title = (link.textContent || "").trim();
      if (!title) {
        return;
      }
      if (!href || href === "#") {
        href = currentPath;
      }
      if (href.startsWith("#") || href.includes("#")) {
        return;
      }
      if (!unique.has(href)) {
        unique.set(href, title);
      }
    });

    const topbar = document.createElement("div");
    topbar.id = "pluss-topbar";

    const brand = document.createElement("a");
    brand.className = "brand";
    brand.href = "index.html";
    brand.textContent = "PlussMic";
    topbar.appendChild(brand);

    const links = document.createElement("div");
    links.className = "top-links";
    topbar.appendChild(links);

    Array.from(unique.entries()).forEach(([href, title]) => {
      const a = document.createElement("a");
      a.href = href;
      a.textContent = title;
      if (href === currentPath || href.endsWith("/" + currentPath)) {
        a.classList.add("active");
      }
      links.appendChild(a);
    });

    document.body.insertBefore(topbar, document.body.firstChild);
  }

  function buildMethodSidebar() {
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
      const groupTitle = cleanHeadingText(text(groupTitleNode)) || "Methods";

      const methodSections = Array.from(root.children).filter((node) => {
        return node.tagName === "SECTION" && node.querySelector(":scope > h3");
      });

      methodSections.forEach((section, index) => {
        if (!section.id) {
          section.id = `method-${groupTitle.toLowerCase().replace(/\W+/g, "-")}-${index + 1}`;
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
          group: groupTitle,
        });
      });
    });

    if (!methodItems.length) {
      return null;
    }

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
        if (child.tagName === "SECTION" && child.querySelector(":scope > h3")) {
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

  function buildPrevNext(methodNav) {
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
    const methodNav = buildMethodSidebar();
    const sectionNav = methodNav ? null : buildSectionSidebar();

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

    buildPrevNext(methodNav || sectionNav);
  });
})();
