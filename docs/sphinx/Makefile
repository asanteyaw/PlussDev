REPO_ROOT := $(abspath $(CURDIR)/../..)
UNIFIED   := python3 $(REPO_ROOT)/docs/scripts/build_unified_docs.py

.PHONY: aggregate html publish latex pdf latexpdf clean

aggregate:
	$(UNIFIED) aggregate

html:
	$(UNIFIED) html --site all

publish: html
	$(REPO_ROOT)/../plussible/scripts/publish-docs.sh $(REPO_ROOT)

latex:
	$(UNIFIED) latex --site root

pdf:
	$(UNIFIED) pdf --site root

latexpdf:
	$(UNIFIED) latexpdf --site root

clean:
	$(UNIFIED) clean --site all
