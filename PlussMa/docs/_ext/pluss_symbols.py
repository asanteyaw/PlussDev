"""Links between the User Guide and the API Reference, by symbol name.

A path such as `../../api/finance/Field/price.md` rots the first time a page moves. This extension
links by *name* instead, from the symbol tables that `apiref.py` already keeps (the
`[[symbol]]` entries of `apiref_manifest*.toml`) and the topic registry (`apiref_topics.toml`):

* the role ``{sym}`Field::price` `` (or ``{sym}`price() <Field::price>` ``) links to that
  symbol's reference page, and *fails the build* with a warning when the name is unknown or
  ambiguous, so a renamed symbol cannot leave a dead link behind;
* on User Guide pages, inline code that is exactly one symbol name of the page's own topic
  (``solve_dfo``, ``DfoOptions``, ``Class::method``) is linked to its reference page at every
  mention, with no markup in the source;
* ``{guide}`topic` `` and ``{reference}`topic` `` link to a topic's other section through the registry;
* every reference page that the guide links to gets a generated "Used in the guide" list, so the
  way back from a function to the chapters that teach it is never maintained by hand.
"""
from __future__ import annotations

import tomllib
from pathlib import Path

from docutils import nodes
from sphinx.util import logging

DOCS = Path(__file__).resolve().parents[1]
log = logging.getLogger(__name__)


class Symbols:
    """Symbol tables of the API reference, per topic."""

    def __init__(self) -> None:
        self.by_qual: dict[str, str] = {}                    # "optim::septima" / "Class::method" -> docname
        self.by_short: dict[str, dict[str, set[str]]] = {}   # topic -> short name -> docnames
        self.topic_of_doc: dict[str, str] = {}               # reference docname -> topic
        self.ambiguous_qual: set[str] = set()

    def add(self, topic: str, key: str, docname: str, qualified: bool) -> None:
        if qualified:
            if key in self.by_qual and self.by_qual[key] != docname:
                self.ambiguous_qual.add(key)
            self.by_qual[key] = docname
        else:
            self.by_short.setdefault(topic, {}).setdefault(key, set()).add(docname)
        self.topic_of_doc[docname] = topic

    def lookup(self, text: str, topic: str | None) -> str | None:
        """The docname a name stands for, or None. A short name must be unique in its topic."""
        if text in self.by_qual and text not in self.ambiguous_qual:
            return self.by_qual[text]
        if topic and topic in self.by_short:
            docs = self.by_short[topic].get(text)
            if docs and len(docs) == 1:
                return next(iter(docs))
        return None


def _load() -> Symbols:
    topics = tomllib.loads((DOCS / "apiref_topics.toml").read_text(encoding="utf-8")).get("topic", [])
    ref_to_topic = {t["reference"][:-3]: t["slug"] for t in topics if t.get("reference")}
    table = Symbols()
    for mp in sorted(DOCS.glob("apiref_manifest*.toml")):
        data = tomllib.loads(mp.read_text(encoding="utf-8"))
        fw_key, fw = next(iter(data["framework"].items()))
        symbols = data.get("symbol", [])
        if not symbols or not str(symbols[0].get("page", "")).startswith("api/"):
            continue                                          # a guide manifest
        topic = ref_to_topic.get(str(fw.get("root_page", ""))[:-5]) or (fw_key if fw_key in ref_to_topic.values() else None)
        if topic is None:
            continue
        for s in symbols:
            docname = s["page"][:-3]
            parts = s["qualname"].split("::")
            name = parts[-1]
            # every trailing form of the qualified name: pluss::subsp::x::f, subsp::x::f, x::f
            for i in range(len(parts) - 1):
                table.add(topic, "::".join(parts[i:]), docname, True)
            table.add(topic, name, docname, False)
            for m in s.get("methods", []):
                table.add(topic, f"{name}::{m}", f"{docname[:-len(name)]}{name}/{m}", True)
    return table


def _topic_of_guide(docname: str, slugs: list[str]) -> str | None:
    if not docname.startswith("manual/"):
        return None
    rest = docname[len("manual/"):]
    for slug in slugs:
        if rest == slug or rest.startswith(slug + "/"):
            return slug
    return None


def _usage(env) -> dict[str, set[str]]:
    if not hasattr(env, "pluss_sym_usage"):
        env.pluss_sym_usage = {}
    return env.pluss_sym_usage


def _reference(env, from_doc: str, target_doc: str, label_nodes: list[nodes.Node]) -> nodes.reference:
    uri = env.app.builder.get_relative_uri(from_doc, target_doc)
    ref = nodes.reference("", "", internal=True, refuri=uri, classes=["pluss-sym"])
    for n in label_nodes:
        ref += n
    return ref


def sym_role(name, rawtext, text, lineno, inliner, options=None, content=None):
    env = inliner.document.settings.env
    table: Symbols = env.app.pluss_symbols
    target, label = text, text
    if text.endswith(">") and "<" in text:
        label, _, rest = text.rpartition("<")
        label, target = label.strip(), rest[:-1].strip()
    topic = _topic_of_guide(env.docname, env.app.pluss_slugs)
    docname = table.lookup(target.removesuffix("()"), topic)
    if docname is None:
        msg = inliner.reporter.warning(
            f"{{sym}}: no unique API reference page for '{target}'", line=lineno)
        return [inliner.problematic(rawtext, rawtext, msg)], [msg]
    _usage(env).setdefault(docname, set()).add(env.docname)
    return [_reference(env, env.docname, docname, [nodes.literal(label, label)])], []


def _topic_link(section: str):
    """Role `{guide}`optim`` / `{reference}`optim``: a link to the topic's other section, resolved through the
    registry (apiref_topics.toml), so a prose cross-link never hard-codes a path. An explicit label may be
    given as `{guide}`the guide <optim>``."""
    def role(name, rawtext, text, lineno, inliner, options=None, content=None):
        env = inliner.document.settings.env
        slug, label = text.strip(), None
        if slug.endswith(">") and "<" in slug:
            label, _, rest = slug.rpartition("<")
            label, slug = label.strip(), rest[:-1].strip()
        topic = env.app.pluss_topics.get(slug)
        page = topic.get(section) if topic else None
        if not page:
            msg = inliner.reporter.warning(f"{{{section}}}: topic '{slug}' has no {section} page", line=lineno)
            return [inliner.problematic(rawtext, rawtext, msg)], [msg]
        if label is None:
            label = f"{topic['title']} {'user guide' if section == 'guide' else 'API reference'}"
        target = page[:-3]
        return [_reference(env, env.docname, target, [nodes.Text(label)])], []
    return role


def autolink(app, doctree):
    """Link inline code on guide pages that is exactly one symbol of the page's own topic."""
    env = app.env
    docname = env.docname
    topic = _topic_of_guide(docname, app.pluss_slugs)
    if topic is None:
        return
    table: Symbols = app.pluss_symbols
    for lit in list(doctree.findall(nodes.literal)):
        parent = lit.parent
        if isinstance(parent, (nodes.reference, nodes.title, nodes.literal_block)):
            continue
        text = lit.astext().strip()
        target = table.lookup(text.removesuffix("()"), topic)
        if target is None or target == docname:
            continue
        _usage(env).setdefault(target, set()).add(docname)
        parent.replace(lit, _reference(env, docname, target, [lit.deepcopy()]))


def purge(app, env, docname):
    usage = _usage(env)
    for target in list(usage):
        usage[target].discard(docname)
        if not usage[target]:
            del usage[target]


def merge(app, env, docnames, other):
    mine = _usage(env)
    for target, docs in _usage(other).items():
        mine.setdefault(target, set()).update(docs)


def updated(app, env):
    """Re-write a reference page whose list of guide chapters changed, though it was not re-read."""
    usage = _usage(env)
    snapshot = getattr(env, "pluss_sym_snapshot", {})
    outdated = [d for d in set(usage) | set(snapshot) if usage.get(d, set()) != snapshot.get(d, set())]
    env.pluss_sym_snapshot = {k: set(v) for k, v in usage.items()}
    return [d for d in outdated if d in env.found_docs]


def used_in_guide(app, doctree, docname):
    docs = sorted(_usage(app.env).get(docname, ()))
    if not docs:
        return
    section = nodes.section(ids=["used-in-the-guide"])
    section += nodes.title("", "Used in the guide")
    items = nodes.bullet_list()
    for d in docs:
        title = app.env.titles.get(d)
        label = title.astext() if title is not None else d
        item = nodes.list_item()
        para = nodes.paragraph()
        para += _reference(app.env, docname, d, [nodes.Text(label)])
        item += para
        items += item
    section += items
    top = next(doctree.findall(nodes.section), None)   # a heading level below the page title
    (top if top is not None else doctree).append(section)


def setup(app):
    app.pluss_symbols = _load()
    app.pluss_slugs = [t["slug"] for t in tomllib.loads((DOCS / "apiref_topics.toml").read_text(encoding="utf-8"))["topic"]]
    app.pluss_topics = {t["slug"]: t for t in tomllib.loads((DOCS / "apiref_topics.toml").read_text(encoding="utf-8"))["topic"]}
    app.add_role("sym", sym_role)
    app.add_role("guide", _topic_link("guide"))
    app.add_role("reference", _topic_link("reference"))
    app.connect("doctree-read", autolink)
    app.connect("env-purge-doc", purge)
    app.connect("env-merge-info", merge)
    app.connect("env-updated", updated)
    app.connect("doctree-resolved", used_in_guide)
    return {"version": "1.0", "parallel_read_safe": False, "parallel_write_safe": True}
