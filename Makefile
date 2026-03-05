.PHONY: build run debug clean setup shell

PROJECT_DIR := $(CURDIR)
IMAGE_NAME := myos_builder
USER_ID := $(shell id -u)
GROUP_ID := $(shell id -g)


setup:
	docker build -t $(IMAGE_NAME) .


build:
	docker run --rm \
		-u $(USER_ID):$(GROUP_ID) \
		-v $(PROJECT_DIR):/project \
		-w /project $(IMAGE_NAME) \
		bash -c "cmake -S . -B build && cmake --build build"


run: build
	python3 scripts/run/run.py --kernel build/kernel.elf --image build/out/os.img 

debug: build
	python3 scripts/run/run.py --kernel build/kernel.elf  --image build/out/os.img --is_debug

shell:
	docker run --rm -it \
		-u $(USER_ID):$(GROUP_ID) \
		-v $(PROJECT_DIR):/project \
		-w /project $(IMAGE_NAME) \
		/bin/bash

clean:
	rm -rf build/
