# CLAUDE.md — YAS Nix

## What
Native GUI wrapper for **Nix** (`nix`). Part of YAS suite.
Status: **scaffolded & unit-tested** — vendored core + adapter + QML shell compile, 3/3 QtTest suites pass (verified cross-compiling on macOS). Pending: build + QA on the real target platform.

## Stack
- C++20 + Qt 6.7+ (Qt Quick / QML), CMake ≥ 3.24
- Only true multi-platform app of the suite. Native windowing via Qt QPA: **cocoa** (macOS), **wayland/xcb** (Linux).
- CLI execution: `QProcess` wrapping `nix`. Never bundle it.
- Architecture: **vendored core copy** (identical across suite, NO shared library by design) + `nix` adapter. Master template: `../yas-core/` local folder (not published). Core fixes must be replicated across repos.

## Target platform
macOS (arm64/x64) + Linux (x64/arm64). NixOS and non-NixOS.

## nix specifics
- User-level profile ops need no root (daemon handles store). Multi-user installs talk to nix-daemon.
- **Two CLI generations**: legacy (`nix-env`) vs new (`nix profile`, `nix search`) — new CLI needs `experimental-features = nix-command flakes`. Decide v1 target: recommend new CLI + detect/enable-guide for experimental flags; fall back to `nix-env` read-only if disabled.
- Key commands: `nix search nixpkgs <q> --json`, `nix profile list --json`, `nix profile install/remove/upgrade`, `nix profile rollback`, `nix store gc`, `nix profile history`, `nix flake update`.
- `--json` available on most new-CLI commands — parse JSON, never text.
- `nix search` cold run evaluates all of nixpkgs (slow, RAM-heavy) — cache results; consider search.nixos.org API (Elasticsearch) for fast metadata.
- Generations/rollback are the differentiator vs every other manager — first-class UI for profile history + rollback.
- Don't touch NixOS system config (`configuration.nix`) — user profiles only. Out of scope.

## Design (see DESIGN.md)
- UI shell: **Teams-style** — icon rail | list panel | detail pane (no in-app title header). Light/dark mode persisted via `YasManager` context property (`src/core/thememanager.*`, QSettings), toggle at rail bottom. Both palettes live in `qml/core/Theme.qml` (`Theme.dark`).
- Dark theme. Base `#222629`, accent **Cyan `#528EBF`**, highlight `#528EBF1A`, text `#F8F8F2` / `#ACADAD`.
- App tag: **NIX**. Fonts: Outfit/Inter (UI), Fira Code or JetBrains Mono (CLI output).

## Conventions
- Conventional Commits (no co-author attribution), feature branches, PRs per CONTRIBUTING.md. Never push to origin without explicit ask.
- Planned layout (mirrors yas-brew, the reference scaffold): `src/core/` (vendored), `src/nixadapter.*`, `src/main.cpp`, `qml/core/` (vendored) + `qml/Main.qml`, `tests/`, `assets/fonts/`, `icons/` (exists), CMakeLists.txt + CMakePresets.json.
- Packaging: nixpkgs derivation / flake (dogfooding) + macOS .dmg.

## Key files
README.md · DESIGN.md · CONTRIBUTING.md · EULA.md · SECURITY.md · icons/
