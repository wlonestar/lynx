SHELL = bash

CLANG = clang-16
CLANG++ = clang++-16

BUILD_DIR = build
DEBUG_BUILD = $(BUILD_DIR)/debug
RELEASE_BUILD = $(BUILD_DIR)/release

INSTALL_DIR = /home/wjl/work/demos/http_server/lynx-1.0.0

.DEFAULT_GOAL := build

config:
	@cmake -G Ninja -B $(DEBUG_BUILD) \
		-DCMAKE_C_COMPILER=$(CLANG) \
		-DCMAKE_CXX_COMPILER=$(CLANG++) \
		-DCMAKE_BUILD_TYPE=Debug
	@cmake -G Ninja -B $(RELEASE_BUILD) \
		-DCMAKE_C_COMPILER=$(CLANG) \
		-DCMAKE_CXX_COMPILER=$(CLANG++) \
		-DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR) \
		-DCMAKE_BUILD_TYPE=Release
	@ln -sf $(RELEASE_BUILD)/compile_commands.json

debug-build: config
	@ninja -C $(DEBUG_BUILD)

release-build: config
	@ninja -C $(RELEASE_BUILD)

build: debug-build release-build

install: release-build
	@ninja -C $(RELEASE_BUILD) install

.PHONY: clean test

test:
	@cd $(DEBUG_BUILD) && ctest
	@cd $(RELEASE_BUILD) && ctest

format:
	@ninja -C $(RELEASE_BUILD) clang-format

clean:
	@rm -rf $(BUILD_DIR) compile_commands.json
