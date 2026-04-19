SHELL        := bash
.SHELLFLAGS  := -euo pipefail -c
.ONESHELL:

STEM      ?= vortex_256x256
MPI_RANKS ?= $(shell nproc)

BUILD          := build
BUILD_SANITIZE := build-sanitize
BUILD_COVERAGE := build-coverage

CMAKE_BASE := cmake -G Ninja \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5

.PHONY: configure \
        build build-analyzer build-simulator build-sanitize build-coverage \
        test test-vector test-analyzer test-simulator test-sanitize test-coverage \
        run-simulator run-analyzer \
        report report-clean \
        clean clean-build clean-sanitize clean-coverage clean-data

# =============================================================================
# Build
# =============================================================================

configure:
	$(CMAKE_BASE) -B $(BUILD) -DCMAKE_BUILD_TYPE=Release

build: configure
	cmake --build $(BUILD)

build-analyzer: configure
	cmake --build $(BUILD) --target analyzer

build-simulator: configure
	cmake --build $(BUILD) --target simulator

build-sanitize:
	$(CMAKE_BASE) -B $(BUILD_SANITIZE) -DCMAKE_BUILD_TYPE=Debug \
	  -DENABLE_CUDA=OFF \
	  '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' \
	  '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined -no-pie'
	cmake --build $(BUILD_SANITIZE)

build-coverage:
	$(CMAKE_BASE) -B $(BUILD_COVERAGE) -DCMAKE_BUILD_TYPE=Debug \
	  -DENABLE_CUDA=OFF \
	  '-DCMAKE_CXX_FLAGS=--coverage' \
	  '-DCMAKE_EXE_LINKER_FLAGS=--coverage'
	cmake --build $(BUILD_COVERAGE)

# =============================================================================
# Test
# =============================================================================

test: build
	ctest --test-dir $(BUILD) --output-on-failure

test-vector: build
	ctest --test-dir $(BUILD) --output-on-failure -R vector

test-analyzer: build
	ctest --test-dir $(BUILD) --output-on-failure -R analyzer

test-simulator: build
	ctest --test-dir $(BUILD) --output-on-failure -R simulator

test-sanitize: build-sanitize
	ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1 \
	  ctest --test-dir $(BUILD_SANITIZE) --output-on-failure

test-coverage: build-coverage
	ctest --test-dir $(BUILD_COVERAGE) --output-on-failure

# =============================================================================
# Run
# =============================================================================

run-simulator: build-simulator
	./$(BUILD)/bins/simulator/simulator configs/$(STEM).toml

run-analyzer: build-analyzer
	./$(BUILD)/bins/simulator/simulator configs/$(STEM).toml
	mpirun -n $(MPI_RANKS) --oversubscribe \
	  ./$(BUILD)/bins/analyzer/analyzer configs/$(STEM).toml

# =============================================================================
# Report
# =============================================================================

report:
	$(MAKE) -C report

report-clean:
	$(MAKE) -C report clean

# =============================================================================
# Clean
# =============================================================================

clean-build:
	rm -rf $(BUILD)

clean-sanitize:
	rm -rf $(BUILD_SANITIZE)

clean-coverage:
	rm -rf $(BUILD_COVERAGE) coverage.info coverage/

clean-data:
	rm -rf data/ *.h5 *.pkl

clean: clean-build clean-sanitize clean-coverage clean-data report-clean
