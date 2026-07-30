REPO_ROOT := $(abspath $(CURDIR)/../../..)
UNIFIED   := python3 $(REPO_ROOT)/docs/scripts/build_unified_docs.py

.PHONY: aggregate html site-html latex pdf latexpdf clean

aggregate:
	$(UNIFIED) aggregate

html:
	$(UNIFIED) html --site all

site-html:
	$(UNIFIED) html --site plussmic

latex:
	$(UNIFIED) latex --site plussmic

pdf:
	$(UNIFIED) pdf --site plussmic

latexpdf:
	$(UNIFIED) latexpdf --site plussmic

clean:
	$(UNIFIED) clean --site all
