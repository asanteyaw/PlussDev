project = 'PlussQit User Docs'
author = 'PlussMa Contributors'

extensions = [
    'myst_parser',
]

source_suffix = {
    '.md': 'markdown',
}

master_doc = 'index'
templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

html_theme = 'alabaster'
html_static_path = ['_static']
html_css_files = ['api_ref.css']
html_js_files = ['layout.js']

myst_enable_extensions = [
    'colon_fence',
    'deflist',
    'fieldlist',
]

latex_engine = 'pdflatex'
latex_documents = [
    ('index', 'plussqit_user_docs.tex', 'PlussQit User Documentation', 'PlussMa Contributors', 'manual'),
]
