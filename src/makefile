# General
# The binary is named with the version: nexus6.1 (derived from VERSION 6.1.0).
# Override with: make build EXE=nexus  (e.g. for OpenBench compatibility)
VERSION  = 6.1.0
# Major.minor for the binary name (e.g. 6.1.0 -> 6.1)
VERSION_SHORT = $(word 1,$(subst ., ,$(VERSION))).$(word 2,$(subst ., ,$(VERSION)))
EXE      = nexus$(VERSION_SHORT)
SRC      = attacks.c bench.c nexus.c bits.c board.c eval.c history.c instrument.c move.c movegen.c movepick.c perft.c random.c \
	search.c see.c tb.c thread.c transposition.c uci.c util.c zobrist.c nn/accumulator.c nn/evaluate.c pyrrhic/tbprobe.c
CC       = clang
MAIN_NETWORK = nexus-9b84c340af7e.nn
EVALFILE = $(MAIN_NETWORK)
# The NNUE network is hosted upstream at the original Berserk networks
# release page. The filename is rebranded to nexus-*, but the binary
# contents and sha256 are identical to the upstream berserk-* release.
# See ATTRIBUTION.md for full provenance and license details.
NETWORK_BASE = https://github.com/jhonnold/berserk-networks/releases/download/networks
DEFS     = -DVERSION=\"$(VERSION)\" -DEVALFILE=\"$(EVALFILE)\" -DNDEBUG

# Flags
STD    = -std=gnu11
LIBS   = -pthread -lm
WARN   = -Wall -Wextra -Wshadow

FLAGS   = $(STD) $(WARN) -g -O3 -flto $(PGOFLAGS) $(DEFS)
M64     = -m64 -mpopcnt
MSSE41  = $(M64) -msse -msse2 -mssse3 -msse4.1
MAVX2   = $(MSSE41) -mbmi -mfma -mavx2
MAVX512 = $(MAVX2) -mavx512f -mavx512bw
ARM64   = -arch arm64

XCRUN   =

# Detecting windows
ifeq ($(shell echo "test"), "test")
	FLAGS += -static
endif

# Detecting Mac
KERNEL := $(shell uname -s)
ifeq ($(KERNEL),Darwin)
	XCRUN = xcrun
endif

# Detecting Apple Silicon (ARM64)
UNAME := $(shell uname -m)
ifeq ($(UNAME), arm64)
    ARCH = arm64
endif

# Setup arch
ifeq ($(ARCH), )
   ARCH = native
endif

ifeq ($(ARCH), native)
	CFLAGS = $(FLAGS) -march=native
else ifeq ($(ARCH), arm64)
	CFLAGS = $(FLAGS) $(ARM64)
else ifeq ($(findstring x86-64, $(ARCH)), x86-64)
	CFLAGS = $(FLAGS) $(M64)
else ifeq ($(findstring sse41, $(ARCH)), sse41)
	CFLAGS = $(FLAGS) $(MSSE41)
else ifeq ($(findstring avx2, $(ARCH)), avx2)
	CFLAGS = $(FLAGS) $(MAVX2)
else ifeq ($(findstring avx512, $(ARCH)), avx512)
	CFLAGS = $(FLAGS) $(MAVX512)
endif

ifeq ($(ARCH), native)
	PROPS = $(shell echo | $(CC) -march=native -E -dM -)
	ifneq ($(findstring __BMI2__, $(PROPS)),)
	ifeq ($(findstring __znver1, $(PROPS)),)
	ifeq ($(findstring __znver2, $(PROPS)),)
	CFLAGS += -DUSE_PEXT
	endif
	endif
	endif
else ifeq ($(findstring -pext, $(ARCH)), -pext)
	CFLAGS += -DUSE_PEXT -mbmi2
endif

openbench: download-network
	$(MAKE) ARCH=avx2 pgo

# Standard release build — max strength, no instrumentation.
# Byte-identical to Nexus 6.0 baseline (bench = 2,811,728 nodes).
release: download-network
	$(MAKE) ARCH=$(ARCH) pgo

build: download-network
	$(MAKE) ARCH=$(ARCH) all

# Profile build — O2 + NEXUS_PROFILE + NEXUS_SUMMARY. Use for hotspot
# analysis and benchdetail. Slightly slower than release but same node count.
profile: download-network
	$(MAKE) ARCH=$(ARCH) EXFLAGS="-DNEXUS_PROFILE -DNEXUS_SUMMARY -O2" all

# Debug build — all instrumentation + ASan/UBSan. Use for development.
# Much slower; do NOT use for strength testing. Uses -O1 (not -O0) because
# the codebase uses bare `inline` (not `static inline`) on several TT
# helpers, which require at least -O1 to get an inline definition emitted.
# LTO is disabled because it conflicts with ASan inlining decisions.
debug: download-network
	$(MAKE) ARCH=$(ARCH) EXFLAGS="-DNEXUS_DEBUG -DNEXUS_PROFILE -DNEXUS_SUMMARY -O1 -g -fno-lto -fsanitize=address,undefined" LIBS="-pthread -lm -fsanitize=address,undefined" all

pgo: download-network
ifeq ($(findstring gcc, $(CC)), gcc)
	$(MAKE) ARCH=$(ARCH) PGOFLAGS="-fprofile-generate=pgo" all

	./$(EXE) bench 13 > pgo.out 2>&1
	grep Results pgo.out

	$(MAKE) ARCH=$(ARCH) PGOFLAGS="-fprofile-use=pgo" all

	@rm -rf pgo pgo.out
else ifeq ($(findstring clang, $(CC)), clang)
	$(MAKE) ARCH=$(ARCH) PGOFLAGS="-fprofile-instr-generate" all

	./$(EXE) bench 13 > pgo.out 2>&1
	grep Results pgo.out

	$(XCRUN) llvm-profdata merge -output=nexus.profdata *.profraw
	$(MAKE) ARCH=$(ARCH) PGOFLAGS="-fprofile-instr-use=nexus.profdata" all

	@rm -rf pgo pgo.out nexus.profdata *.profraw
else
	@echo "PGO builds not supported for $(CC)"
endif

all:
	$(CC) $(CFLAGS) $(EXFLAGS) $(SRC) $(LIBS) -o $(EXE)

download-network:
	@if [ "$(EVALFILE)" = "$(MAIN_NETWORK)" ]; then \
	echo "Using the current best network: $(EVALFILE)"; \
	if test -f "$(EVALFILE)"; then \
	echo "File already downloaded"; \
	elif hash wget 2>/dev/null; then \
	echo "Downloading $(EVALFILE) with wget"; wget -qO- $(NETWORK_BASE)/berserk-`echo $(EVALFILE) | sed 's/^nexus-//'` > $(EVALFILE); \
	elif hash curl 2>/dev/null; then \
	echo "Downloading $(EVALFILE) with curl"; curl -fskL $(NETWORK_BASE)/berserk-`echo $(EVALFILE) | sed 's/^nexus-//'` > $(EVALFILE); \
	fi; \
	if test -f "$(EVALFILE)"; then \
	if hash shasum 2>/dev/null; then \
	if [ "$(EVALFILE)" = "nexus-"`shasum -a 256 $(EVALFILE) | cut -c1-12`".nn" ]; then \
	echo "Downloaded network $(EVALFILE) and verified"; \
	echo "NOTE: the network binary is identical to the upstream berserk-* release;"; \
	echo "      only the filename prefix has been rebranded for Nexus 6.0."; \
	else \
	echo "Downloaded network $(EVALFILE) failed validation"; \
	exit 1; \
	fi; \
	elif hash sha256sum 2>/dev/null; then \
	if [ "$(EVALFILE)" = "nexus-"`sha256sum $(EVALFILE) | cut -c1-12`".nn" ]; then \
	echo "Downloaded network $(EVALFILE) and verified"; \
	echo "NOTE: the network binary is identical to the upstream berserk-* release;"; \
	echo "      only the filename prefix has been rebranded for Nexus 6.0."; \
	else \
	echo "Downloaded network $(EVALFILE) failed validation"; \
	exit 1; \
	fi; \
	else \
	echo "Downloaded network $(EVALFILE), but unable to verify"; \
	fi; \
	else \
	echo "Unable to download network: $(EVALFILE)"; \
	exit 1; \
	fi; \
	elif test -f "$(EVALFILE)"; then \
	echo "Using network: $(EVALFILE)"; \
	else \
	echo "Unknown network: $(EVALFILE)"; \
	fi;

clean:
	rm -f $(EXE)
