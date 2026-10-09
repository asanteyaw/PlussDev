REPO_ROOT := $(abspath $(CURDIR)/../../..)
UNIFIED   := python3 $(REPO_ROOT)/docs/scripts/build_unified_docs.py

.PHONY: aggregate html site-html latex pdf latexpdf clean

aggregate:
	$(UNIFIED) aggregate

html:
	$(UNIFIED) html --site all

site-html:
	$(UNIFIED) html --site plusstils

latex:
	$(UNIFIED) latex --site plusstils

pdf:
	$(UNIFIED) pdf --site plusstils

latexpdf:
	$(UNIFIED) latexpdf --site plusstils

clean:
	$(UNIFIED) clean --site plusstils
