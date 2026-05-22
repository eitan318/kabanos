.PHONY: build run debug clean setup shell docs

PROJECT_DIR := $(CURDIR)
IMAGE_NAME := myos_builder
BUILD_DIR := build
OUT_DIR := $(BUILD_DIR)

# --- OS Detection & Cross-Platform Configuration ---
ifeq ($(OS),Windows_NT)
    SHELL := cmd.exe
    USER_ID := 1000
    GROUP_ID := 1000
    RM_RF := rmdir /s /q
    OPEN_DOCS := start docs/html/index.html
else
    USER_ID := $(shell id -u)
    GROUP_ID := $(shell id -g)
    RM_RF := rm -rf
    ifeq ($(shell uname),Darwin)
        OPEN_DOCS := open docs/html/index.html
    else
        OPEN_DOCS := xdg-open docs/html/index.html
    endif
endif

# --- Recipes ---

setup:
	docker build -t $(IMAGE_NAME) .

# Flattened single-line recipes bypass multi-line tracking bugs perfectly
build:
	docker run --rm -u $(USER_ID):$(GROUP_ID) -v "$(PROJECT_DIR)":/project -w /project $(IMAGE_NAME) bash -c "cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug && cmake --build $(BUILD_DIR)"

run: build
	python3 scripts/run/run.py --kernel $(BUILD_DIR)/kernel/kernel.elf --image $(OUT_DIR)/os.img

debug: build
	python3 scripts/run/run.py --kernel $(BUILD_DIR)/kernel/kernel.elf --image $(OUT_DIR)/os.img --is_debug

shell:
	docker run --rm -it -u $(USER_ID):$(GROUP_ID) -v "$(PROJECT_DIR)":/project -w /project $(IMAGE_NAME) /bin/bash

docs:
	doxygen Doxyfile
	$(OPEN_DOCS)

clean:
	$(RM_RF) $(BUILD_DIR)