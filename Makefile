.PHONY: build run debug clean setup shell docs

PROJECT_DIR := $(CURDIR)
IMAGE_NAME := myos_builder
USER_ID := $(shell id -u)
GROUP_ID := $(shell id -g)
BUILD_DIR := build
OUT_DIR := $(BUILD_DIR)


setup:
	docker build -t $(IMAGE_NAME) .


build:
	docker run --rm \
		-u $(USER_ID):$(GROUP_ID) \
		-v $(PROJECT_DIR):/project \
		-w /project $(IMAGE_NAME) \
		bash -c "cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug && cmake --build $(BUILD_DIR)"


run: build
	python3 scripts/run/run.py --kernel $(BUILD_DIR)/kernel.elf --image $(OUT_DIR)/os.img 


debug: build
	python3 scripts/run/run.py --kernel $(BUILD_DIR)/kernel.elf --image $(OUT_DIR)/os.img --is_debug &

shell:
	docker run --rm -it \
		-u $(USER_ID):$(GROUP_ID) \
		-v $(PROJECT_DIR):/project \
		-w /project $(IMAGE_NAME) \
		/bin/bash

docs:
	doxygen Doxyfile
	open docs/html/index.html

clean:
	rm -rf build/
