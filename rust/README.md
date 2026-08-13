# Rust migration

The shipping desktop application uses a hybrid boundary: Tauri/Rust owns the
window, settings, persistence and tray lifecycle, while the proven C++ engine
is statically linked into the same process for SDL controller polling and live
mapping. There is no helper process or adjacent C++ executable.

Current slices:

- `ui-model`: serializable UI settings and the hold-to-enable fly-mouse rule.
- `gyro-engine`: the first Rust port of the C++ LOCAL-space gyro mouse path:
  cutoff, hold-button gate, velocity-based sensitivity, time integration and
  subpixel accumulation. `tests/run_parity.ps1` feeds identical simulated
  frames to a C++ reference and Rust, then checks every output frame.
- `tauri-app`: the Tauri 2 + React application, native persistence, tray
  restore, autostart, per-device profiles, and a C ABI bridge to the embedded
  C++ core.

Window close is intentionally intercepted: the title-bar close button hides
the main window, and only the tray menu's Quit item terminates the process.
- `input-engine`: deterministic press/release, single/double timing and simple
  simultaneous-key parsing used for Rust-side behavior tests.

Run `build-portable.ps1` from PowerShell to build the static C++ core, compile
the web assets and create `out/portable/JoyShockMapper.exe`.

Planned order:

1. Keep the C ABI narrow and preserve C++ behavior with simulated integration
   tests.
2. Move isolated algorithms only after C++/Rust parity tests pass.
3. Perform the final HID, Bluetooth and gyro calibration check on physical
   left/right Joy-Con hardware before publishing a hardware-certified release.
