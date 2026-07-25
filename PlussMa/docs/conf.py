project = 'PlussMa'
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
    # Internal engineering notes (parity benchmarks/scorecards/todos against
    # a reference implementation) -- not end-user documentation, kept out of
    # the built site.
    'optima_clone_parity_benchmarks.md', 'optima_clone_parity_scorecard.md',
    'optima_clone_parity_todo.md', 'optima_near_parity_todo.md',
    'optima_parity_matrix.md', 'optima_pathology_clone_parity_benchmarks.md',
    'optima_structured_clone_parity_benchmarks.md',
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
    ('index', 'plussma_package_docs.tex', 'PlussMa Package Documentation', 'PlussMa Contributors', 'manual'),
]
