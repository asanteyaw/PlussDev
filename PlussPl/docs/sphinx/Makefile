REPO_ROOT := $(abspath $(CURDIR)/../../..)
UNIFIED   := python3 $(REPO_ROOT)/docs/scripts/build_unified_docs.py

.PHONY: aggregate html site-html latex pdf latexpdf clean

aggregate:
	$(UNIFIED) aggregate

html:
	$(UNIFIED) html --site all

site-html:
	$(UNIFIED) html --site plusspl

latex:
	$(UNIFIED) latex --site plusspl

pdf:
	$(UNIFIED) pdf --site plusspl

latexpdf:
	$(UNIFIED) latexpdf --site plusspl

clean:
	$(UNIFIED) clean --site all
