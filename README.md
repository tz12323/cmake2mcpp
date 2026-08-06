# cmake2mcpp

Convert CMake projects into **mcpp.toml**.

`cmake2mcpp` analyzes a CMake project and generates an equivalent `mcpp.toml` configuration, making it easier to migrate existing CMake-based projects to **mcpp**.

> **Status:** Early development. Some CMake features may not be supported yet.

## Features

* Parse CMake projects
* Generate `mcpp.toml`
* Preserve project information
* Convert targets and their dependencies
* Header-only library detection
* Modern C++17 implementation

## Build

### Requirements

* C++17 or later
* CMake 3.26+
* A C++ compiler (GCC, Clang, or MSVC)

### Dependencies

This project uses the following open-source libraries:

| Library       | Version | License      |
| ------------- | ------- | ------------ |
| toml++        | 3.4.0   | MIT          |
| nlohmann/json | 3.12.0  | MIT          |
| CLI11         | 2.7.2   | BSD-3-Clause |

## Usage

```bash
cmake2mcpp -b <build-directory> -o <output-directory>
```

Example:

```bash
cmake2mcpp -b fmt/build -o fmt
```

The generated project will contain an `mcpp.toml` file.

## License

This project is licensed under the MIT License.

### Third-party Licenses

This project uses the following third-party libraries:

* toml++ (MIT License)
* nlohmann/json (MIT License)
* CLI11 (BSD-3-Clause License)
