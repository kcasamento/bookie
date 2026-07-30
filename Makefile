.PHONY: build test test-sanitize build-test build-sanitize benchmark build-benchmark prof-benchmark

build:
	cmake --preset debug && cmake --build --preset debug

build-test:
	cmake --preset debug && cmake --build --preset debug --target orderbook_tests

test: build-test
	ctest --test-dir build/debug --output-on-failure

build-sanitize:
	cmake --preset sanitize && cmake --build --preset sanitize --target orderbook_tests

test-sanitize: build-sanitize
	ctest --test-dir build/sanitize --output-on-failure

build-benchmark:
	cmake --preset release && cmake --build --preset release --target orderbook_benchmarks

benchmark: build-benchmark
	./build/release/benchmark/orderbook_benchmarks

prof-benchmark:
	rm -rf bench.trace && \
	cmake --preset release -DCMAKE_CXX_FLAGS="-march=native -fno-omit-frame-pointer" && \
	cmake --build --preset release --target orderbook_benchmarks && \
	xctrace record --template 'Time Profiler' --output bench.trace --launch -- ./build/release/benchmark/orderbook_benchmarks
