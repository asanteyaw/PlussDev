project = 'PlussQit'
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
    # Internal engineering notes (patch logs, codegen internals, parity
    # roadmaps) -- not end-user documentation, kept out of the built site.
    'PATCHES.md', 'ROADMAP.md', 'ParityRoadmap.md',
    'Codegen.md', 'Codegen_MethodSurface.md', 'CodeM_MethodRoutingPlan.md',
    'Torch90_TrancheCycle.md',
    'Patch*.md',
    # Internal authoring and contract records are not end-user pages.
    'API_Doc_Authoring_Standard.md', 'Series_Doc_Authoring_Standard.md',
    'NOTE_CLOSED_TYPE_CONTRACT.md', 'README.md',
]

html_theme = 'alabaster'
html_static_path = ['_static']
html_css_files = ['api_ref.css']
html_js_files = ['api_nav.js', 'layout.js']

myst_enable_extensions = [
    'colon_fence',
    'deflist',
    'fieldlist',
]

latex_engine = 'pdflatex'
latex_documents = [
    ('index', 'plussqit_package_docs.tex', 'PlussQit Package Documentation', 'PlussMa Contributors', 'manual'),
]
