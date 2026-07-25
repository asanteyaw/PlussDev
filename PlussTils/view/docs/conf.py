project = 'PlussTils'
author = 'PlussMa Contributors'

extensions = [
    'myst_parser',
]

source_suffix = {
    '.md': 'markdown',
}

master_doc = 'index'
exclude_patterns = [
    '_build', 'Thumbs.db', '.DS_Store',
    # Internal engineering roadmap (patch planning, local dev-machine file
    # paths, competitor-analysis notes) -- not end-user documentation.
    'matlab_popup_viewer_roadmap.md',
]

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
    ('index', 'plusstils_package_docs.tex', 'PlussTils Package Documentation', 'PlussMa Contributors', 'manual'),
]
