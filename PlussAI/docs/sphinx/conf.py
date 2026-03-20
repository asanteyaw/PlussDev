import importlib.util

project = 'PlussAI NodeJ + Lymph API Docs'
author = 'Pluss Contributors'

extensions = [
    'myst_parser',
]

source_suffix = {
    '.md': 'markdown',
}

master_doc = 'index'
templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'README.md']

if importlib.util.find_spec('pydata_sphinx_theme'):
    html_theme = 'pydata_sphinx_theme'
    html_theme_options = {
        'show_toc_level': 2,
        'show_nav_level': 2,
        'navigation_depth': 3,
        'secondary_sidebar_items': ['page-toc'],
    }
else:
    html_theme = 'alabaster'

html_static_path = ['_static']
html_css_files = ['api_ref.css']
pygments_style = "sphinx"
pygments_dark_style = "native"

myst_enable_extensions = [
    'colon_fence',
    'deflist',
    'fieldlist',
]

latex_engine = 'pdflatex'
latex_documents = [
    ('index', 'plussai_nodej_lymph_api_docs.tex', 'PlussAI NodeJ + Lymph API Documentation', 'Pluss Contributors', 'manual'),
]
