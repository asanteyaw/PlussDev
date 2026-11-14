project = 'Pluss'
author = 'PlussMa Contributors'

extensions = [
    'myst_parser',
]


def _plain_role(name, rawtext, text, lineno, inliner, options=None, content=None):
    """PlussMa pages link to their guide / reference through roles that its own extension resolves; the aggregate
    hub renders the topic name as plain code instead."""
    from docutils import nodes
    return [nodes.literal(text, text)], []


def setup(app):
    for role in ('guide', 'reference', 'sym'):
        app.add_role(role, _plain_role)

source_suffix = {
    '.md': 'markdown',
}

master_doc = 'index'
templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', '_local/*/snippets/**']

html_theme = 'alabaster'
html_static_path = ['_static']
html_css_files = ['api_ref.css']
html_js_files = ['layout.js']

myst_enable_extensions = [
    'colon_fence',
    'deflist',
    'fieldlist',
]
myst_heading_anchors = 3

myst_url_schemes = ("http", "https", "mailto", "ftp", "file")

latex_engine = 'pdflatex'
latex_documents = [
    ('index', 'pluss_documentation.tex', 'Pluss Documentation', 'PlussMa Contributors', 'manual'),
]
