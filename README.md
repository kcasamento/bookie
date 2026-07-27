# bookie

Progressive C++ exercises building toward a production-style order book engine, in the spirit of high-frequency trading systems. See [ROADMAP.md](ROADMAP.md) for the full phase breakdown, and `exercises/NN-topic/README.md` for each exercise's spec.

## Toolchain
- CMake 3.21+
- GCC (Clang works too)

## Build

Three configure presets are available:

| Preset     | Purpose                                  |
|------------|-------------------------------------------|
| `debug`    | Fast iteration, no optimization           |
| `sanitize` | Debug build + ASan/UBSan                  |
| `release`  | `-O3 -march=native`                       |

```
cmake --preset debug
cmake --build --preset debug
```

Executables land under `build/<preset>/exercises/<exercise-dir>/`, mirroring the source layout. E.g.:

```
./build/debug/exercises/01-value-semantics-and-raii/ex01_value_semantics
```

Swap `debug` for `sanitize` or `release` to build under those configs.

## Structure
```
exercises/
  01-value-semantics-and-raii/
    README.md   <- exercise spec
    main.cpp    <- your implementation
```

New exercises get added as `exercises/NN-topic/` and registered in `exercises/CMakeLists.txt`.
