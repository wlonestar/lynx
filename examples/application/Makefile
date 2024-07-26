SHELL = bash

BUILD_DIR = build

.DEFAULT_GOAL := build

config:
	@cmake -G Ninja -B $(BUILD_DIR) \
		-DCMAKE_C_COMPILER=clang \
		-DCMAKE_CXX_COMPILER=clang++ \
		-DCMAKE_BUILD_TYPE=Release

build: config
	@ninja -C $(BUILD_DIR)

.PHONY: clean

clean:
	@rm -rf $(BUILD_DIR)
