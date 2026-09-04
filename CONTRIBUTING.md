# Contributing to testpulse-gtest

## Setup

Requires CMake 3.16+ and a C++17 compiler. On Linux, `libcurl` dev
headers are needed for the CLI: `sudo apt-get install libcurl4-openssl-dev`
(macOS ships `curl` already; no extra install needed).

## Running tests

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

## Test-driven development

This project follows red-green-refactor: write a failing test before any
production code, watch it fail for the expected reason, then implement
the minimal code to make it pass.

Several layers of tests exist side by side:

- **Unit tests** for the tagging/attachment library (`tests/case_test.cc`,
  `tests/attach_test.cc`) and the CLI (`cli/tests/config_test.cc`,
  `cli/tests/report_reader_test.cc`, `cli/tests/attachments_test.cc`,
  `cli/tests/submit_test.cc`) — the submit-logic tests inject a fake
  `HttpClient`, so they need no network access.
- **Real end-to-end tests** (`tests/e2e_test.cc`,
  `cli/tests/submit_e2e_test.cc`) spawn real compiled fixture binaries
  and the actual `testpulse-gtest` CLI binary as subprocesses, and (for
  the CLI) talk to a real HTTP server (`cli/tests/stub_server.hpp`, a
  minimal POSIX-socket server, not a mock) — this is what actually
  caught real, non-obvious findings during this project's own
  development, including confirming gtest's real `RecordProperty()`
  XML output shape against gtest's own documentation being wrong about
  it.

When adding a fixture binary under `tests/fixtures/`, link it against the
`testpulse::testpulse` target directly (via `add_executable` +
`target_link_libraries`) rather than `FetchContent`-ing this repo from
itself — there's no packaging-specific auto-wiring step in this design
(unlike some ecosystems' build-tool plugin systems) that a plain
in-repo link would fail to exercise, so a real cross-repo `FetchContent`
round-trip isn't needed to validate fixture behavior.

## Testing the FetchContent consumption path for real

The README's own install snippet is the actual contract downstream
consumers rely on. To verify it works as written against a real tagged
release (not just this repo's own in-tree build), point a scratch CMake
project's `FetchContent_Declare` at a real tag from this repo and build
against it — an `add_subdirectory` shortcut within this repo would not
catch a packaging mistake (e.g. a header not actually being installed
where the include path expects it).

## License

MIT
