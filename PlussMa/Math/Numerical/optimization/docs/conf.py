project = 'Optima User Docs'
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

myst_enable_extensions = [
    'colon_fence',
    'deflist',
    'fieldlist',
]

latex_engine = 'pdflatex'
latex_documents = [
    ('index', 'optima_user_docs.tex', 'Optima User Documentation', 'PlussMa Contributors', 'manual'),
]
