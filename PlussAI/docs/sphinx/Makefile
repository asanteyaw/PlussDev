REPO_ROOT := $(abspath $(CURDIR)/../../..)
UNIFIED   := python3 $(REPO_ROOT)/docs/scripts/build_unified_docs.py

.PHONY: aggregate html site-html latex pdf latexpdf clean

aggregate:
	$(UNIFIED) aggregate

html:
	$(UNIFIED) html --site all

site-html:
	$(UNIFIED) html --site plussai

latex:
	$(UNIFIED) latex --site plussai

pdf:
	$(UNIFIED) pdf --site plussai

latexpdf:
	$(UNIFIED) latexpdf --site plussai

clean:
	$(UNIFIED) clean --site all
