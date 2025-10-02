# GodotEOS GDExtension

This folder contains the GDExtension native plugin for integrating Epic Online Services (EOS) with Godot Engine. It provides the native library, build scripts, and a demo project to validate the integration.

Purpose
-------
- Host native code and build scripts for the GodotEOS GDExtension.
- Provide an example/demo Godot project that loads the extension and includes EOS sample assets.
- Provide setup utilities to prepare the extension for local builds and local testing.

Contents
--------
- `SConstruct` - SCons build script used to build the extension for supported platforms.
- `setup_gdextension.ps1` - Windows PowerShell setup script that prepares environment, copies the needed SDK and binaries into demo paths.
- `setup_godot_cpp_module.ps1` - Helper script to configure `godot-cpp` bindings (if used).
- `extension_api.json` - Reflection metadata used by Godot for the GDExtension API.
- `demo/` - Minimal/demo Godot project with scenes, scripts, and a `bin/` folder containing sample build artifacts.
- `eos_samples/` - Epic-provided sample projects and tooling (kept for reference).
- `eos_sdk/put_eos_sdk_here` - Placeholder where you should place the EOS SDK.

Quick prerequisites
-------------------
- Godot Engine 4.x (editor for running the demo)
- SCons (build tool used by the Godot build system and `godot-cpp`)
- Visual Studio (MSVC) toolchain on Windows or a suitable C/C++ toolchain on Linux/macOS
- Epic Online Services SDK (place the SDK files under `eos_sdk/` as described below)

Putting the EOS SDK in place
----------------------------
1. Download the EOS SDK from Epic's Developer portal.
2. Extract the SDK and copy relevant `Bin`, `Include`, `Lib`, and `Tools` folders into `godot_epic_extension/eos_sdk/` or update the build scripts to point to your SDK location.
3. On Windows, ensure the demo `bin/` contains `EOSSDK-Win64-Shipping.dll` alongside the built `.gdextension` if you plan to run the demo from the editor.

Build instructions (Windows PowerShell)
------------------------------------
Open a PowerShell prompt in this folder and run one of the SCons targets below. The workspace includes VS Code tasks for these commands (see top-level tasks in the repository) but the direct commands are shown here.

Debug build (Windows x64):
```
scons platform=windows target=template_debug arch=x86_64
```

Release build (Windows x64):
```
scons platform=windows target=template_release arch=x86_64
```

Other platforms
- Linux x64: `scons platform=linux target=template_debug` (or `template_release`)
- macOS: `scons platform=macos target=template_debug arch=universal`

Notes about builds
------------------
- The build system expects `godot-cpp` and EOS SDK include/libs to be available. If you use a different layout, update `SConstruct` accordingly.
- The resulting artifacts will be placed under the `demo/bin/` folder when using the provided demo configuration.
- Use the VS Code tasks in the workspace (Build GDExtension - Debug/Release) to run these builds from the editor.

Running the demo
----------------
1. Ensure the built `.gdextension` and any required EOS runtime DLLs are in `godot_epic_extension/demo/bin/` (or copy them into the demo project's `bin/` folder).
2. Open `godot_epic_extension/demo/project.godot` in Godot Editor 4.x.
3. Enable the `GodotEOS` plugin (if required) under Project Settings → Plugins.
4. Run the `main.tscn` scene to see the demo UI and test EOS-related flows.

Setup helper scripts
--------------------
- `.\setup_gdextension.ps1` - Designed to automate common steps on Windows: copy EOS DLLs into the demo, prepare build environment, and optionally run SCons. Run with ExecutionPolicy Bypass if needed:

```
powershell -ExecutionPolicy Bypass -File .\setup_gdextension.ps1
```

- `.\setup_godot_cpp_module.ps1` - Assists with building or configuring the `godot-cpp` bindings (used by older Godot native modules and sometimes by GDExtension setups). Use only if you are customizing the binding build.

API and extension metadata
--------------------------
The `extension_api.json` file contains the API metadata used by the GDExtension system. If you update exposed native classes or methods, regenerate or update this file as part of the build process so the Godot runtime sees the updated API.

Troubleshooting
---------------
- Plugin not loading in Godot:
	- Verify that the `.gdextension` file is in the expected `bin/` location and that the `extension_api.json` file is present alongside it.
	- On Windows, check for missing DLL dependencies (`EOSSDK-Win64-Shipping.dll` etc.). Place them in the same directory as the `.gdextension` or in a directory on `PATH`.
- Build failures with SCons:
	- Ensure the correct Visual Studio command-line tools or a matching compiler toolchain are available.
	- If includes/libs for EOS are not found, update `SConstruct` or set environment variables that point to the SDK location.
- Runtime crashes / missing symbols:
	- Make sure the build architecture matches the Godot editor architecture (x86_64 vs x86_32).

Testing & validation
--------------------
- The demo project provides a quick smoke test for the extension. Use it to verify the extension loads and basic signals/methods are reachable from GDScript.
- For CI, consider adding an automated build step that runs `scons` and then validates the resulting `.gdextension` by launching a headless Godot export and checking plugin load success.

Development notes
-----------------
- This repository currently contains a mock/demo implementation for many EOS flows. See the root `README.md` for higher-level usage patterns and `docs/` for design/analysis notes.
- If you modify C++ sources, update `godot-cpp` bindings (if used) and rebuild before testing in the editor.

Contributing
------------
- Fork and send pull requests against the `main` branch.
- Keep native API changes backwards compatible where possible and update `extension_api.json` and the demo accordingly.

License
-------
This project is licensed under the MIT license. See the repository `LICENSE` file for details.

Where to look next
------------------
- Root `README.md` — higher-level project overview and demo usage examples.
- `docs/` — design notes and EOS analysis files (e.g. `EOS_SDK_Analysis.md`).
- `plan.md` — development roadmap and planned features.

If you'd like, I can also:
- add a short CONTRIBUTING.md with native build conventions,
- add a minimal smoke-test script to validate built `.gdextension` files,
- or wire the PowerShell setup script to copy SDK artifacts automatically.

Enjoy developing with Godot + EOS!

