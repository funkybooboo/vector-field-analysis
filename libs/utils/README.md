# utils

Small shared utilities with no project-specific dependencies.

## Contents

- **`formatBytes.hpp`** — `Utils::formatBytes(bytes)` formats a byte count as a human-readable string (`1.4 MB`, `512 KB`, etc.)
- **`toml_include.hpp`** — single include point for `toml++` with project-wide TOML settings (`TOML_EXCEPTIONS=1`)

## Usage

Link against the `utils` CMake target, or include headers directly since all utilities are header-only.
