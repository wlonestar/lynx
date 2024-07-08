SHELL = bash

CLANG = /usr/bin/clang
CLANG++ = /usr/bin/clang++

BUILD_DIR = build
DEBUG_BUILD = $(BUILD_DIR)/debug
RELEASE_BUILD = $(BUILD_DIR)/release

.DEFAULT_GOAL := build

config:
	@cmake -G Ninja -B $(DEBUG_BUILD) \
		-DCMAKE_C_COMPILER=$(CLANG) -DCMAKE_CXX_COMPILER=$(CLANG++) \
		-DCMAKE_BUILD_TYPE=Debug
	@cmake -G Ninja -B $(RELEASE_BUILD) \
		-DCMAKE_C_COMPILER=$(CLANG) -DCMAKE_CXX_COMPILER=$(CLANG++) \
		-DCMAKE_BUILD_TYPE=Release
	@ln -sf $(RELEASE_BUILD)/compile_commands.json $(BUILD_DIR)/compile_commands.json

build: config
	@ninja -C $(DEBUG_BUILD)
	@ninja -C $(RELEASE_BUILD)

.PHONY: clean test format

test:
	@cd $(DEBUG_BUILD) && ctest
	@cd $(RELEASE_BUILD) && ctest

format:
	@find . -not -path '*/$(BUILD_DIR)/*' -regex '.*\.\(cpp\|hpp\|h\|c\)' \
		-exec clang-format -i --verbose {} \;

clean:
	@rm -rf $(BUILD_DIR) compile_commands.json
