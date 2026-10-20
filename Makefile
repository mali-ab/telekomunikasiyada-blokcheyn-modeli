# Convenience targets for the console build. The desktop UI is built through
# CMake as well; see README.md for the Qt command.

CMAKE ?= cmake
BUILD_DIR ?= build-console

.PHONY: all configure test clean

all: configure
	$(CMAKE) --build $(BUILD_DIR)

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -DBUILD_DESKTOP=OFF -DBUILD_CONSOLE=ON

test: all
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	$(CMAKE) -E rm -rf $(BUILD_DIR)
