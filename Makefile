ifeq ($(wildcard .git),)
	$(error You need to download this project by git! Aborting!)
endif


all: demo

space := $(subst ,, )


# Parsing
# -------------------------------------------- 
FIRST_ARG := $(firstword $(MAKECMDGOALS))
ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))

MAKE_PID := $(shell echo $$PPID)


j := $(shell ps T | sed -n "s|.*$(MAKE_PID).*$(MAKE) *\(-j\) *\([0-9][0-9]*\).*|\2|p")
j_clang_tidy := $(or $(j),4)

NINJA_BIN := ninja
ifndef NO_NINJA_BUILD
	NINJA_BUILD := $(shell $(NINJA_BIN) --version 2>/dev/null)
	ifndef NINJA_BUILD
		NINJA_BIN := ninja-build
		NINJA_BUILD := $(shell $(NINJA_BIN) --version 2>/dev/null)
	endif
endif

ifdef NINJA_BUILD
	TAROX_CMAKE_GENERATOR := Ninja
	TAROX_MAKE := $(NINJA_BIN)

	ifdef VERBOSE
		TAROX_MAKE_ARGS := -v
	else
		TAROX_MAKE_ARGS :=
	endif

	ifneq ($(j),)
		TAROX_MAKE_ARGS := $(TAROX_MAKE_ARGS) -j$(j)
	endif
else
	ifdef SYSTEMROOT
		TAROX_CMAKE_GENERATOR := "MSYS\ Makefiles"
	else
		TAROX_CMAKE_GENERATOR := "Unix\ Makefiles"
	endif

	j := $(or $(j),4)
	TAROX_MAKE = $(MAKE)
	TAROX_MAKE_ARGS := -j$(j) --no-print-directory
endif

SRC_DIR := $(shell dirname "$(realpath $(lastword $(MAKEFILE_LIST)))")

CMAKE_ARGS ?=

ifdef EXTERNAL_MODULES_LOCATION
	override CMAKE_ARGS += -DEXTERNAL_MODULES_LOCATION:STRING=$(EXTERNAL_MODULES_LOCATION)
endif

ifdef TAROX_CMAKE_BUILD_TYPE
	override CMAKE_ARGS += -DCMAKE_BUILD_TYPE=$(TAROX_CMAKE_BUILD_TYPE)
endif

define cmake-build
	$(eval override CMAKE_ARGS += -DTAROX_CONFIG_NAME=$(1))
	@$(eval BUILD_DIR = "$(SRC_DIR)/build/$(1)")
	@$(call cmake-cache-check)
	@if [ $(TAROX_CMAKE_GENERATOR) = "Ninja" ] && [ -e $(BUILD_DIR)/Makefile ]; then rm -rf $(BUILD_DIR); fi
	@if [ $(TAROX_CMAKE_GENERATOR) = "Ninja" ] && [ ! -f $(BUILD_DIR)/build.ninja ]; then rm -rf $(BUILD_DIR); fi
	@if [ ! -e $(BUILD_DIR)/CMakeCache.txt ] || [ $(CMAKE_CACHE_CHECK) ]; then \
		mkdir -p $(BUILD_DIR) \
		&& cd $(BUILD_DIR) \
		&& cmake "$(SRC_DIR)" -G"$(TAROX_CMAKE_GENERATOR)" \
		-D CMAKE_EXPORT_COMPILE_COMMANDS=ON $(CMAKE_ARGS)\
		|| (rm -rf $(BUILD_DIR)); \
	fi
	@cmake --build $(BUILD_DIR) -- $(TAROX_MAKE_ARGS) $(ARGS)
endef

define cmake-cache-check
	@$(eval CACHED_CMAKE_OPTIONS = $(shell cd $(BUILD_DIR) 2>/dev/null && cmake -L 2>/dev/null | sed -n 's|\([^[:blank:]]*\):[^[:blank:]]*\(=[^[:blank:]]*\)|\1\2|gp'))
	@$(eval DESIRED_CMAKE_OPTIONS = $(shell echo $(CMAKE_ARGS) | sed -n 's|-D\([^[:blank:]]*=[^[:blank:]]*\)|\1|gp'))
	@$(eval VERIFIED_CMAKE_OPTIONS = $(foreach option,$(DESIRED_CMAKE_OPTIONS),$(strip $(findstring $(option)$(space),$(CACHED_CMAKE_OPTIONS)))))
	@$(eval CMAKE_CACHE_CHECK = $(if $(findstring $(DESIRED_CMAKE_OPTIONS),$(VERIFIED_CMAKE_OPTIONS)),,y))
endef

ALL_CONFIG_TARGETS := $(shell find boards -maxdepth 3 -mindepth 2 -name '*.taroxconfig' -print | sed -e 's|boards\/||' | sed -e 's|\.taroxconfig||' | sed -e 's|\/|_|g' | sort)

CONFIG_TARGETS_DEFAULT := $(patsubst %_default,%,$(filter %_default,$(ALL_CONFIG_TARGETS)))
$(ALL_CONFIG_TARGETS):
	@$(call cmake-build,$@)

$(CONFIG_TARGETS_DEFAULT):
	@$(call cmake-build,$@_default)

%:
	$(if $(filter $(FIRST_ARG),$@),\
		$(error "Make target $@ not found. It either does not exist or $@ can't be the first argument. Use 'make list' to show all configuration targets."),@#)

.PHONY: help list list_config_targets

help:
	@echo "Tarox build — quick reference"
	@echo ""
	@echo "  make / make all     Default build (see the \`all\` target in this Makefile)"
	@echo "  make <config>       Build a board configuration; output under build/<config>/"
	@echo "  make <config> flash  Flash build/<config>/tarox.bin only (no rebuild; needs st-flash)"
	@echo "  make <config> build-flash  Build if needed, then flash"
	@echo "  make <config> flash-openocd  Flash only via OpenOCD (see CMake cache: TAROX_OPENOCD_*)"
	@echo "  make list           List all configurations (same as make list_config_targets)"
	@echo ""
	@echo "  Put the configuration first (e.g. make demo_default flash, not make flash ...)."
	@echo "  Config names come from boards/**/*.taroxconfig: path components become"
	@echo "  underscores, then the filename without .taroxconfig. For default.taroxconfig,"
	@echo "  you may omit the _default suffix; \`make list\` shows that as [_default]."
	@echo ""
	@echo "Optional environment / variables:"
	@echo "  TAROX_CMAKE_BUILD_TYPE      e.g. Debug or Release"
	@echo "  EXTERNAL_MODULES_LOCATION    Extra modules path (see CMake / project docs)"
	@echo "  VERBOSE=1                   Verbose Ninja output"
	@echo "  NO_NINJA_BUILD=1            Use Unix Makefiles instead of Ninja"
	@echo "  CMake flash options: reconfigure with -DTAROX_FLASH_ADDR=0x08000000, etc."

list list_config_targets:
	@for targ in $(patsubst %_default,%[_default],$(ALL_CONFIG_TARGETS)); do echo $$targ; done