# Copyright 2024 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Cyril Koenig <cykoenig@iis.ee.ethz.ch>

# HERO_ARTIFACTS_ROOT is where the data is saved while
# HERO_ARTIFACTS_DIR is where the hash are generated, they can be the same
HERO_ARTIFACTS_ROOT ?=

TERM_GREEN := "\033[92m"
TERM_YELLOW := "\033[0;33m"
TERM_RED := "\033[0;31m"
TERM_NC := "\033[0m"

# Generate sensitivity list for artifact
$(HERO_ARTIFACTS_DIR)/.generated_env_%: FORCE
	@if [ -z "$(HERO_ARTIFACTS_DATA_$(*))" ]; then \
		echo "Error: HERO_ARTIFACTS_DATA_$(*) missing"; \
		exit 1; \
	fi
	@echo "Sensitivity variables for target: $*" > $@
	@echo $(HERO_ARTIFACTS_VARS_$*) >> $@
	@echo "Sensitivity folder for target: $*" >> $@
	@sha256sum $(HERO_ARTIFACTS_FILES_$*) | awk '{print $$1}' >> $@
.PRECIOUS: $(HERO_ARTIFACTS_DIR)/.generated_env_%

# Hash sensitivity list
$(HERO_ARTIFACTS_DIR)/.generated_sha256_%: $(HERO_ARTIFACTS_DIR)/.generated_env_%
	@sha256sum $< | awk '{print $$1}' > $@
.PRECIOUS: $(HERO_ARTIFACTS_DIR)/.generated_sha256_%

# Try to load an artifact (if enabled)
ifeq ($(HERO_ARTIFACTS_ROOT),)
hero-load-artifacts-%:
	@echo -e $(TERM_YELLOW)"[HERO] Artifacts management disabled"$(TERM_NC)
else
hero-load-artifacts-%: $(HERO_ARTIFACTS_DIR)/.generated_sha256_%
	@if [ -z "$(HERO_ARTIFACTS_ROOT_$(*))" ]; then \
		echo "Error: HERO_ARTIFACTS_ROOT_$(*) missing"; \
		exit 1; \
	fi
	@if [ -d "$(HERO_ARTIFACTS_ROOT_$(*))/`cat $<`" ]; then \
		echo -e $(TERM_GREEN)"Fetching $(*) from $(HERO_ARTIFACTS_ROOT_$(*))/`cat $<`"$(TERM_NC); \
		mkdir -p $(HERO_ARTIFACTS_DATA_$(*)); \
		cp -r $(HERO_ARTIFACTS_ROOT_$(*))/`cat $<`/* $(HERO_ARTIFACTS_DATA_$(*)); \
	else \
		echo -e "Artifact $(HERO_ARTIFACTS_ROOT_$(*))/`cat $<` not found - building from scratch ..."; \
	fi
endif

# Try to save an artifact (if enabled)
ifeq ($(HERO_ARTIFACTS_ROOT),)
hero-save-artifacts-%:
	;
else
hero-save-artifacts-%: $(HERO_ARTIFACTS_DIR)/.generated_sha256_%
	@if [ ! -d "$(HERO_ARTIFACTS_ROOT_$(*))/`cat $<`" ]; then \
		echo -e $(TERM_GREEN)"Saving $(*) to $(HERO_ARTIFACTS_ROOT_$(*))/`cat $<`"$(TERM_NC); \
		cp -r $(HERO_ARTIFACTS_DATA_$(*)) $(HERO_ARTIFACTS_ROOT_$(*))/`cat $<`; \
		chmod -R g+rw $(HERO_ARTIFACTS_ROOT_$(*))/`cat $<`; \
	fi
endif


FORCE: ;
