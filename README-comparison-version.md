# Windows comparison identity

Await configuration **before** `setFeedURL`, which initializes WinSparkle and
can start a scheduled check immediately:

```dart
if (Platform.isWindows) {
  await autoUpdater.setAppBuildVersion(installedReleaseIdentity); // 1.0.7+18
}
await autoUpdater.setScheduledCheckInterval(21600);
await autoUpdater.setFeedURL(feedUrl);
```

The App supplies an immutable installed identity matching both appcast version
fields exactly. Keep `1.0.7+18` for existing clients; `+18` participates in native
comparison. This API does not change ProductVersion or FileVersion.

The existing channel `dev.leanflutter.plugins/auto_updater` accepts
`setAppBuildVersion` with `{'buildVersion': '<string>'}`. Windows returns
`invalid_argument` for missing, non-string, empty, invalid UTF-8, or embedded-NUL
values; `already_initialized` for every change after initialization. Repeated
valid calls before initialization are allowed; the last one wins. Calls run on
the platform thread and the stored UTF-16 value is passed immediately before
the first `win_sparkle_init`. Callers omitting this API retain WinSparkle's legacy
ProductVersion fallback. On other platforms the Dart setter throws
`UnsupportedError`; existing APIs are unchanged. Native errors surface as
`PlatformException` with their stable code.

Run Dart tests in `packages/auto_updater_platform_interface` and
`packages/auto_updater`. While developing a fork, override its platform interface
locally so facade tests use the matching API. Publish the platform/native commit
before pinning it from the facade; publish the facade before App consumers can
resolve its exact commit from a remote clone.

On Windows, configure and run native argument/UTF-16/order/fallback tests with:

```powershell
cmake -S packages/auto_updater_windows/windows/test -B build/native-tests `
  -DFLUTTER_ENGINE_DIR=C:/path/to/flutter/bin/cache/artifacts/engine/windows-x64
cmake --build build/native-tests --config Debug
ctest --test-dir build/native-tests -C Debug --output-on-failure
```

The native comparator fixture is the unmodified comparator section from
WinSparkle v0.8.1, attributed and SHA-256 recorded in its header. It is compiled
as C++ instead of substituting publisher/Python version comparison. It can also
run portably with `clang++ -std=c++17` on its test source.

For an actual WinSparkle DLL probe after building the native fixtures:

```powershell
python packages/auto_updater_windows/windows/test/run_identity_acceptance.py `
  --fixture-dir build/native-tests/Debug
```

It checks equality, N-1 discovery and installed-newer suppression under manual
and native scheduled checks against both high and low EXE version resources,
plus legacy fallback controls. Every case uses a fresh process and isolated
registry preferences. Payload download, installation, and actual App relaunch
acceptance remain separate.
