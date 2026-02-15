# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.2.0] - CLI Support

### Added

- Command-line interface (CLI) via `text-similarity` command using `commander`.
- `similarity` command to calculate similarity scores directly from the terminal.
- `distance` command to calculate string distances from the terminal.
- `batch` command to process multiple string pairs from a JSON file.
- `algorithms` command to list all available algorithms with descriptions.
- Support for all algorithm options via CLI flags (`--algorithm`, `--preprocessing`, `--ignore-case`, `--ngram-size`, `--threshold`, `--alpha`, `--beta`, `--prefix-weight`).
- JSON and plain text output formats (`--format`).
- `bin/` entry point registered in `package.json` for global installation.

## [1.0.0] - Initial Release

This is the first release of the `text-similarity-node` library, which provides high-performance text similarity algorithms in Node.js.

## [1.1.0] - Configurable max string length

### Added

- `maxStringLength` option in `AlgorithmConfig` to configure the maximum allowed input string length (default: 100,000 bytes ≈ 100KB).
- Pass `maxStringLength` via `setGlobalConfiguration()` to allow comparing large documents that exceed the default limit.
- New tests verifying configurable string length behavior.
- Updated README with usage example for large document comparison.

## [1.0.1] - Add postinstall script

This release adds a postinstall script to the `text-similarity-node` library informing users about an upcoming Rust + WebAssembly version that will improve performance and memory usage.
