REPO_ROOT := $(abspath $(CURDIR)/../..)
UNIFIED   := python3 $(REPO_ROOT)/docs/scripts/build_unified_docs.py

.PHONY: aggregate html site-html latex pdf latexpdf clean

aggregate:
	$(UNIFIED) aggregate

html:
	$(UNIFIED) html --site all

site-html:
	$(UNIFIED) html --site plussqit

latex:
	$(UNIFIED) latex --site plussqit

pdf:
	$(UNIFIED) pdf --site plussqit

latexpdf:
	$(UNIFIED) latexpdf --site plussqit

clean:
	$(UNIFIED) clean --site all
