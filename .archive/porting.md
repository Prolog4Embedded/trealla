# Porting

This file contains documentation regarding the process of porting
trealla for an embedded environment.

## CMake

As a first step the existing Makefile is replaced with a CMake setup
to simplify targeting different architectures and environments.

For the most part this step has been a close 1:1 translation
with two small differences.

- Omitting some setup regarding for this project irrelevant targets (e.g. WASI)
- Adding a small foundation for usage of different toolchains / targets

This step is the first step beeing taken, to allow
for a clearer comparison between each step of the migration
by providing a mostly stable build process along the way.

> [!TIP]
> The new `Makefile` contains shortcuts/examples for using
> the CMake setup.

## Next steps

1. Identify and remove irrelevant features
2. Identify and abstract os dependencies
3. Implement abstraction layers for different targets

