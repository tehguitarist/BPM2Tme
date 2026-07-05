# Build Rules

## Toolchain

- **CMake only** (no Projucer/Xcode project — those were removed in favour of this). `cmake_minimum_required(VERSION 3.15)` on line 1.
- C++17. macOS deployment target 11.0, arm64-only (matches the plugin's original Apple Silicon-only build).
- JUCE 8+ as a submodule under `libs/JUCE`, pinned to a release tag (currently `8.0.14`).

## Submodule setup

```bash
git submodule update --init --recursive
```

## Build commands

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target BPM2Time_AU      # AU (macOS only)
cmake --build build --target BPM2Time_VST3    # VST3 (all platforms)
cmake --build build                           # everything
```
`COPY_PLUGIN_AFTER_BUILD TRUE` installs built plugins to the user plugin folders on build
(`~/Library/Audio/Plug-Ins/Components` and `.../VST3` on macOS). Logic caches AU components —
**bump the project `VERSION`** in `CMakeLists.txt` to force a rescan after changes, or
remove/re-add the plugin.

## Plugin identity — do not change without a reason

`PLUGIN_MANUFACTURER_CODE Manu` and `PLUGIN_CODE I7r3` are carried over unchanged from the
original `.jucer` project. Existing users already have `BPM2Time.component` built with these
codes (see the `Release`/`Release_1.1` GitHub releases) — a DAW identifies an AU by
type+subtype+manufacturer, so changing either code makes hosts treat new builds as a completely
different plugin, breaking saved sessions that reference the old one. Same goes for
`BUNDLE_ID com.LeighPierce.BPM2Time`.

## CI / release (GitHub Actions)

- **`ci.yml`** — builds AU+VST3 (macOS) / VST3 (Windows, Linux) on every push to `main` and every
  PR. Validates the macOS AU with `auval` and the VST3 on all three platforms with `pluginval`.
- **`release.yml`** — `workflow_dispatch` ONLY (no push trigger, so a release is never cut by
  accident): builds on all three OSes, signs + notarizes the macOS bundles and installer,
  packages a per-platform installer (see below) alongside a raw zip, and publishes a draft
  GitHub Release tagged `v<CMakeLists version>`.
- **auval on CI gotcha:** a freshly-copied `.component` isn't registered with the
  `AudioComponentRegistrar` on a clean runner, so `auval` fails with "didn't find the component" /
  `-50`. Both workflows bounce the registrar (`killall -9 AudioComponentRegistrar`) and retry.
- **pluginval** validates the VST3 bundle directly (no OS registration needed), so it's the
  cross-platform check; `auval` remains the AU-specific one on macOS.

### macOS signing + notarization

Requires an active Apple Developer Program membership ($99/yr). Nine GitHub Actions secrets are
already configured on this repo (`gh secret list`), all referenced by `release.yml`'s `macos`
job — six for signing the AU/VST3 bundles, three more for signing the `.pkg` installer itself (a
**separate cert type** — "Developer ID Installer", not "Application"; `codesign` and
`productsign` each only accept their own cert type):

| Secret | What it is |
|---|---|
| `APPLE_CERT_P12_BASE64` | `base64 -i DeveloperIDApplication.p12` of the exported Developer ID Application cert |
| `APPLE_CERT_PASSWORD` | the password the `.p12` was exported with |
| `APPLE_SIGNING_IDENTITY` | e.g. `Developer ID Application: Leigh Pierce (TEAMID)` — copy exactly from `security find-identity -v -p codesigning` |
| `APPLE_TEAM_ID` | the 10-character Apple Developer Team ID |
| `APPLE_ID` | the Apple ID email used for notarization |
| `APPLE_APP_SPECIFIC_PASSWORD` | an app-specific password for that Apple ID (generate at appleid.apple.com → Sign-In and Security, NOT the main password) |
| `APPLE_INSTALLER_CERT_P12_BASE64` | `base64 -i DeveloperIDInstaller.p12` of the exported **Developer ID Installer** cert |
| `APPLE_INSTALLER_CERT_PASSWORD` | the password that `.p12` was exported with |
| `APPLE_INSTALLER_SIGNING_IDENTITY` | e.g. `Developer ID Installer: Leigh Pierce (TEAMID)` — copy exactly from `security find-identity -v -p basic` (Installer certs don't show under `-p codesigning`) |

Re-set a cert + its password **together** if either ever needs changing, to avoid a
stale-pairing mismatch (`SecKeychainItemImport: MAC verification failed` almost always means the
`.p12` and password secrets don't actually match).

### Installers

`installer/macos/`, `installer/windows/`, `installer/linux/` hold per-platform installer
scripts/configs, wired into `release.yml`'s "Build installer" step in each platform job:

- **macOS** (`build_installer.sh` + `Distribution.xml`) — a `.pkg` built via `pkgbuild` +
  `productbuild`, with a choice screen for AU vs VST3 (both selected by default). With the three
  `APPLE_INSTALLER_*` secrets configured, `release.yml` also `productsign`s, notarizes, and
  staples the `.pkg` itself.
- **Windows** (`BPM2Time.nsi`, NSIS) — VST3 only (no AU on Windows). `makensis` ships on
  `windows-latest` runners but is **not on PATH** — located via `Get-Command` / common install
  paths, falling back to `choco install nsis`.
- **Linux** (`build_deb.sh` + `control`) — VST3 only (no AU on Linux), a `.deb` via `dpkg-deb`
  (preinstalled on `ubuntu-latest`).

All three scripts take `<version> [artefacts-dir] [output-dir]` and expect the relevant build
targets already built.

## Validation gates (do not skip ahead)

- Both plugin formats build on all three platforms; CI is green.
- The macOS AU passes `auval -v aufx I7r3 Manu`.
- The VST3 passes `pluginval --strictness-level 5`.
- Manually load both formats in a real DAW (Logic for AU; Reaper/Ableton for VST3) before
  publishing a non-draft release — tempo sync and manual-BPM mode should both behave identically
  to the previous Projucer-built releases.
