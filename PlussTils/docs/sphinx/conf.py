project = 'PlussTils'
author = 'Pluss Contributors'

extensions = [
    'myst_parser',
]

source_suffix = {
    '.md': 'markdown',
}

master_doc = 'index'
exclude_patterns = [
    '_build',
    '_local',
    'Thumbs.db',
    '.DS_Store',
]

html_theme = 'alabaster'
html_theme_options = {
    'sidebar_collapse': False,
    'globaltoc_includehidden': True,
}
html_static_path = ['_static']
html_css_files = ['api_ref.css']
html_js_files = ['api_nav.js', 'layout.js']

myst_enable_extensions = [
    'colon_fence',
    'deflist',
    'fieldlist',
]
myst_heading_anchors = 3

latex_engine = 'pdflatex'
latex_documents = [
    ('index', 'plusstils_docs.tex', 'PlussTils Documentation', 'Pluss Contributors', 'manual'),
]
