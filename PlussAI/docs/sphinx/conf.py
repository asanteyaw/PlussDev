project = 'PlussAI'
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

html_theme = 'alabaster'
html_theme_options = {
    'sidebar_collapse': False,
    'globaltoc_includehidden': True,
}
html_sidebars = {
    '**': ['globaltoc.html', 'searchbox.html'],
}
html_static_path = ['_static']
html_css_files = ['api_ref.css']
html_js_files = ['api_nav.js', 'layout.js']
pygments_style = "sphinx"
pygments_dark_style = "native"

myst_enable_extensions = [
    'colon_fence',
    'deflist',
    'fieldlist',
]

latex_engine = 'pdflatex'
latex_documents = [
    ('index', 'plussai_api_docs.tex', 'PlussAI API Documentation', 'Pluss Contributors', 'manual'),
]
