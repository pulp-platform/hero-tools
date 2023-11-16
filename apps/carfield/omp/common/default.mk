#

ROOT := $(patsubst %/,%, $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
TARGET_HOST  = riscv64-hero-linux-gnu
TARGET_DEV   = riscv32-hero-unknown-elf
REPO_ROOT    = $(shell realpath ../../../..)
SNRT_INSTALL = ../snruntime

ifndef HERO_INSTALL
$(error HERO_INSTALL is not set)
endif

# Buildroot contains the GCC toolchain
BR_OUTPUT_DIR ?= $(realpath ../../../../cva6-sdk/buildroot/output/)
RISCV          = $(BR_OUTPUT_DIR)/host
RV64_SYSROOT   = $(RISCV)/riscv64-buildroot-linux-gnu/sysroot

# We compile with clang but need the GCC Linux sysroot / newlib (hero) sysroot
CFLAGS        += --gcc-toolchain=$(RISCV) --sysroot=$(RV64_SYSROOT) --hero-sysroot=$(HERO_INSTALL)/riscv32-unknown-elf

# Binaries
LLVM_INSTALL = $(HERO_INSTALL)
CC := $(HERO_INSTALL)/bin/clang
LINK := $(HERO_INSTALL)/bin/ld.lld
COB := $(HERO_INSTALL)/bin/clang-offload-bundler
DIS := $(HERO_INSTALL)/bin/llvm-dis
HOP := $(HERO_INSTALL)/bin/hc-omp-pass
GCC := $(HERO_INSTALL)/bin/$(TARGET_HOST)-gcc
HOST_OBJDUMP := $(RISCV)/bin/riscv64-buildroot-linux-gnu-objdump
DEV_OBJDUMP := $(HERO_INSTALL)/bin/llvm-objdump

# CFLAGS
ARCH_HOST = host-$(TARGET_HOST)
ARCH_DEV = openmp-$(TARGET_DEV)

CFLAGS_COMMON +=  -O0 -v -debug -save-temps=obj
CFLAGS_PULP += $(CFLAGS_COMMON) -target $(TARGET_DEV)
CFLAGS += -target $(TARGET_HOST) $(CFLAGS_COMMON)
CFLAGS_COMMON += -fopenmp=libomp
CFLAGS += -fopenmp-targets=$(TARGET_DEV)
LDFLAGS_COMMON ?= $(ldflags)
LDFLAGS_PULP += $(LDFLAGS_COMMON)
LDFLAGS += $(LDFLAGS_COMMON) -L$(REPO_ROOT)/sw/libomp/lib
LDFLAGS += --hero-T=/scratch2/cykoenig/development/carfield_perf/safety_island/sw/pulp-runtime/kernel/chips/carfield/link.ld
LDFLAGS += -hero-L /scratch2/cykoenig/development/carfield_perf/safety_island/sw/tests/runtime_omp/build/
LDFLAGS += -hero-l omptarget_runtime
LDFLAGS += --hero-ld-path=$(HERO_INSTALL)/bin/ld.lld
LDFLAGS += --ld-path=$(RISCV)/bin/riscv64-buildroot-linux-gnu-ld

INCPATHS += -I$(REPO_ROOT)/sw/libhero/include
INCPATHS += -I$(ROOT) -include hero_64.h
LIBPATHS += $(RV64_LIBPATH)
LIBPATHS ?=

APP = $(shell basename `pwd`)
EXE = $(APP)
SRC = $(CSRCS)

DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

AS_ANNOTATE_ARGS ?=

.PHONY: all exe clean
.PRECIOUS: %.ll

all: $(DEPS) $(EXE) $(EXE).dis $(EXE).snitch.dis

# Compile heterogeneous source and split into host/device .ll
%.ll: %.c $(DEPDIR)/%.d | $(DEPDIR)
	@echo "CC     <= $<"
	echo "SNRT_INSTALL=$(SNRT_INSTALL) $(CC) $(debug) -c -emit-llvm -S $(DEPFLAGS) $(CFLAGS) $(INCPATHS) $<"
	SNRT_INSTALL=$(SNRT_INSTALL) $(CC) $(debug) -c -emit-llvm -S $(DEPFLAGS) $(CFLAGS) $(INCPATHS) $<
	@echo "COB    <= $<"
	@$(COB) -inputs=$@ -outputs="$(<:.c=-host.ll),$(<:.c=-dev.ll)" -type=ll -targets="$(ARCH_HOST),$(ARCH_DEV)" -unbundle

.PRECIOUS: %-dev.OMP.ll
%-dev.OMP.ll: %.ll
	@echo "HOP    <= $<"
	@cp $(<:.ll=-dev.ll) $(@:.OMP.ll=.TMP.1.ll)
	@LLVM_INSTALL=$(HERO_INSTALL)/ $(HOP) $(@:.OMP.ll=.TMP.1.ll) OmpKernelWrapper "HERCULES-omp-kernel-wrapper" $(@:.OMP.ll=.TMP.2.ll)
	@LLVM_INSTALL=$(HERO_INSTALL)/ $(HOP) $(@:.OMP.ll=.TMP.2.ll) OmpHostPointerLegalizer "HERCULES-omp-host-pointer-legalizer" $(@:.OMP.ll=.TMP.3.ll)
	@cp $(@:.OMP.ll=.TMP.3.ll) $@

.PRECIOUS: %-host.OMP.ll
%-host.OMP.ll: %.ll
	@echo "HOP    <= $<"
	@LLVM_INSTALL=$(HERO_INSTALL)/ $(HOP) $(<:.ll=-host.ll) OmpKernelWrapper "HERCULES-omp-kernel-wrapper" $(@:.OMP.ll=.TMP.1.ll)
	@cp $(@:.OMP.ll=.TMP.1.ll) $@

%-out.ll: %-host.OMP.ll %-dev.OMP.ll
	@echo "COB    <= $<"
	@$(COB) -inputs="$(@:-out.ll=-host.OMP.ll),$(@:-out.ll=-dev.OMP.ll)" -outputs=$@ -type=ll -targets="$(ARCH_HOST),$(ARCH_DEV)"

exeobjs := $(patsubst %.c, %-out.ll, $(SRC))
$(EXE): $(exeobjs)
	@echo "CCLD   <= $<"
	SNRT_INSTALL=$(SNRT_INSTALL) $(CC) $(debug) $(LIBPATHS) $(CFLAGS) $(exeobjs) $(LDFLAGS) -v -o $@
	echo "done"

$(EXE).dis: $(EXE)
	@echo "OBJDUMP <= $<"
	@$(HOST_OBJDUMP) -d $^ > $@

# $<.rodata_off in the skip argument to `dd` is the offset of the first section in the ELF file
# determined by readelf -S $(EXE).
$(EXE).snitch.dis: $(EXE)
	@echo "OBJDUMP (device) <= $<"
	@llvm-readelf -S $(EXE) | grep '.rodata' | awk '{print "echo $$[0x"$$4" - 0x"$$5"]"}' | bash > $<.rodata_off
	@llvm-readelf -s $^ | grep '\s\.omp_offloading.device_image\>' \
			| awk '{print "dd if=$^ of=$^_riscv.elf bs=1 count=" $$3 " skip=$$[0x" $$2 " - $$(< $<.rodata_off)]"}' \
			| bash \
			&& $(DEV_OBJDUMP) -S $^_riscv.elf > $@

# Dep
$(DEPDIR):
	@mkdir -p $@

DEPFILES := $(CSRCS:%.c=$(DEPDIR)/%.d)
$(DEPFILES):

include $(wildcard $(DEPFILES))

# Phony
clean::
	-rm -vf __hmpp* $(EXE) *~ *.bc *.dis *.elf *.i *.lh *.lk *.ll *.o *.s *.slm a.out*
	-rm -rvf $(DEPDIR)
	-rm -vf *-host-llvm *-host-gnu

.PHONY: upload
upload: $(EXE)
	[ "${DEPLOY_FILES}" ] && scp ${DEPLOY_FILES} root@hero-vcu128-02.ee.ethz.ch:/root || echo "Sending only binary..."
	scp $? root@172.31.182.94:/root

.PHONY: install
install: $(EXE)
	cp $? $(REPO_ROOT)/board/common/overlay/root
