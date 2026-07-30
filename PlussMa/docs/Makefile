REPO_ROOT := $(abspath $(CURDIR)/../..)
UNIFIED   := python3 $(REPO_ROOT)/docs/scripts/build_unified_docs.py

.PHONY: aggregate html site-html latex pdf latexpdf clean

aggregate:
	$(UNIFIED) aggregate

html:
	$(UNIFIED) html --site all

site-html:
	$(UNIFIED) html --site plussma

latex:
	$(UNIFIED) latex --site plussma

pdf:
	$(UNIFIED) pdf --site plussma

latexpdf:
	$(UNIFIED) latexpdf --site plussma

clean:
	$(UNIFIED) clean --site all
