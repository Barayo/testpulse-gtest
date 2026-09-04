# testpulse-gtest

A header-only C++ library plus a small CLI for reporting Google Test
(gtest) results into [TestPulse](https://github.com/Barayo/TestPulse) —
tags a test with a TestPulse case key, and submits the JUnit XML report
`--gtest_output=xml` already produces (matching each tagged test to an
existing case) via `testpulse-gtest submit`.

> **Requires C++17 and CMake 3.16+**, matching gtest 1.18.x's own current
> floor exactly.

Unlike most languages, gtest already has a built-in mechanism for writing
custom key/value data into its JUnit XML output —
[`RecordProperty()`](https://google.github.io/googletest/advanced.html#logging-additional-information)
— and it produces exactly the `<properties><property name="..."
value="..."/></properties>` shape TestPulse's importer expects
(confirmed against gtest's own real unittest fixture source; gtest's own
documentation page shows a stale, incorrect example for this). So this
library doesn't generate or rewrite any XML itself — it's a thin,
zero-reflection call-through to gtest's own already-correct writer, plus
a submit CLI that reads gtest's output.

## Install

Add to your `CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(
    testpulse_gtest
    GIT_REPOSITORY https://github.com/Barayo/testpulse-gtest.git
    GIT_TAG v1.0.0  # always pin to a released tag, never `main`
)
set(TESTPULSE_GTEST_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TESTPULSE_GTEST_BUILD_CLI OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(testpulse_gtest)

target_link_libraries(your_test_target PRIVATE testpulse::testpulse)
```

**Always pin `GIT_TAG` to a specific released tag, never a branch name
like `main`.** Unlike npm/PyPI/Maven Central/NuGet, CMake's `FetchContent`
has no immutable, content-addressed version resolution of its own — a
`GIT_TAG main` example resolves to whatever the default branch happens to
point at when your build runs, with no version pinning at all. This is a
real supply-chain footgun, not a style preference.

**`TESTPULSE_GTEST_BUILD_CLI` defaults to `ON`.** Without setting it to
`OFF` as shown above, `FetchContent_MakeAvailable` also fetches `libcurl`/
`nlohmann/json` and builds the `submit` CLI inside your own build tree —
harmless, but unexpected if you only wanted the header-only tagging
library. You don't need to build the CLI yourself at all: download a
prebuilt binary from the
[latest release](https://github.com/Barayo/testpulse-gtest/releases)
instead (see below).

Download `testpulse-gtest` (the submit CLI) from the
[latest release](https://github.com/Barayo/testpulse-gtest/releases) for
your platform (Linux/macOS/Windows binaries are attached to every
release).

## Tag your tests

```cpp
#include <testpulse/testpulse.hpp>

TEST(LoginTest, Succeeds) {
    testpulse::Case("LOGIN-42", testpulse::WithPlatform("linux"), testpulse::WithTags({"smoke"}));
    // ...
}
```

`testpulse::Case` needs no test handle passed in — gtest's own
`RecordProperty()` is a `static` member function.

Run your suite exactly as normal, requesting XML output:

```sh
your_test_binary --gtest_output=xml:report.xml
```

Then submit the report:

```sh
testpulse-gtest submit --file report.xml --url https://testpulse.example --project LOGIN --token "$TESTPULSE_TOKEN"
```

## Attach screenshots/files

```cpp
TEST(LoginTest, FailsWithBadPassword) {
    testpulse::Case("LOGIN-43");
    auto screenshot = TakeScreenshot();
    testpulse::Attach("LOGIN-43", screenshot, "failure.png", "image/png");
}
```

`Attach` only accepts a case key the *currently-executing* test has
itself declared via `Case` (identified through gtest's own
`current_test_info()`) — an attachment under a case key belonging to a
different test throws `testpulse::TestPulseError`, as does a call made
outside any active test (e.g. from `main()` or global setup). Only
`image/png`, `image/jpeg`, and `image/webp` are accepted. Multiple
`Attach` calls under the same case key within one test are all preserved
(e.g. capturing two screenshots). Attachments are written to a
`.testpulse/` scratch directory on disk (the test binary and the later
`testpulse-gtest submit` invocation are separate processes), which
`submit`'s `--dir` flag (default `.`) searches recursively.

## Configuration

`submit` resolves each setting by checking, in order: the CLI flag, then
the environment variable. There is no third, config-file-backed tier.

| Setting | Flag | Env var |
|---|---|---|
| API base URL | `--url` | `TESTPULSE_URL` |
| API token | `--token` | `TESTPULSE_TOKEN` |
| Project key | `--project` | `TESTPULSE_PROJECT` |
| Fail on unmatched | `--fail-on-unmatched` | `TESTPULSE_FAIL_ON_UNMATCHED` |
| Attachments search root | `--dir` (default `.`) | — |
| Preview without submitting | `--dry-run` | — |

**Use `TESTPULSE_TOKEN` in CI**, not `--token` — a flag value can end up
in shell history or process listings; an environment variable set from a
CI secret does not.

## Exit code policy

| Response | Behavior |
|---|---|
| `201` all matched | exit `0`; summary printed |
| `207` some unmatched | exit `0` by default (unmatched keys printed, points at `--fail-on-unmatched`); exit non-zero if `--fail-on-unmatched` is set |
| network/auth/4xx/5xx error | always exits non-zero, unconditionally |

## Dry run

```sh
testpulse-gtest submit --file report.xml --url ... --project LOGIN --token ... --dry-run
```

Fetches existing case keys via a read-only `GET /api/v1/projects/{project}/cases`
and previews which tagged tests in the report would match, without
submitting anything. Exits `0` if the preview fetch itself succeeds,
regardless of the preview's match/no-match content.

## Known v1 limitations

- Google Test only — no Catch2.
- `TEST`/`TEST_F` only — no `BENCHMARK` integration.
- No `vcpkg`/Conan distribution — `FetchContent` only.

## License

MIT
