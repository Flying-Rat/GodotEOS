# GodotEOS Setup Guide

A comprehensive guide to set up and use the GodotEOS GDExtension for Epic Online Services (EOS) integration in Godot 4.3+.

## Prerequisites

### Required Software
- **Godot Engine 4.3** (tested with Godot 4.3+)
- **Visual Studio 2019/2022** or **MinGW** (Windows)
- **SCons** build system
- **Git** for cloning repositories

### Required SDKs
- **Epic Online Services (EOS) SDK** - Download from [Epic Developer Portal](https://dev.epicgames.com/)

## Installation

### Step 1: Clone the Repository

```bash
git clone https://github.com/Flying-Rat/GodotEOS.git
cd GodotEOS
```

### Step 2: Set Up EOS SDK

1. Download the EOS SDK from the [Epic Developer Portal](https://dev.epicgames.com/)
2. Extract the SDK and copy the contents to the `eos_sdk` directory:

```
GodotEOS/
├── eos_sdk/                    # ← Create this directory
│   ├── Bin/                   # EOS SDK binaries
│   ├── Include/               # EOS SDK headers
│   ├── Lib/                   # EOS SDK libraries
│   ├── Tools/                 # EOS SDK tools
│   └── put_eos_sdk_here       # Marker file (already present)
└── godot_eos_extension/       # Extension source code
```

**Important**: Make sure the EOS SDK files are placed directly in the `eos_sdk` directory structure as shown above.

### Step 3: Build the Extension

Navigate to the extension directory:

```bash
cd godot_eos_extension
```

Build for your target platform:

**Windows:**
```bash
scons platform=windows
```

**Linux:**
```bash
scons platform=linux
```

**macOS:**
```bash
scons platform=macos
```

The compiled extension will be placed in `demo/bin/`.

### Step 4: Copy EOS Runtime Libraries

Copy the EOS runtime libraries to your project's binary directory:

**Windows:**
```bash
# Copy from eos_sdk/Bin/ to demo/bin/
copy ..\eos_sdk\Bin\EOSSDK-Win64-Shipping.dll demo\bin\
```

**Linux:**
```bash
# Copy from eos_sdk/Bin/ to demo/bin/
cp ../eos_sdk/Bin/libEOSSDK-Linux-Shipping.so demo/bin/
```

**macOS:**
```bash
# Copy from eos_sdk/Bin/ to demo/bin/
cp ../eos_sdk/Bin/libEOSSDK-Mac-Shipping.dylib demo/bin/
```

## Project Setup

### Step 1: Copy Extension to Your Project

Copy the built extension files to your Godot project:

```
YourGodotProject/
├── addons/
│   └── godoteos/            # ← Create this directory
│       ├── bin/               # Copy demo/bin/ contents here
│       └── godoteos.gdextension  # Copy from demo/
└── your_game_files/
```

### Step 2: Enable the Extension

1. Open your Godot project
2. Go to **Project → Project Settings → Plugins**
3. Find "GodotEOS" and enable it

### Step 3: Configure EOS

The GodotEOS plugin includes an `EpicOS` singleton that handles EOS initialization and provides a high-level API for EOS features. The singleton is automatically added as an autoload when you enable the plugin.

To initialize EOS, call the `EpicOS.initialize()` method with your configuration:

```gdscript
func _ready():
    # Configure EOS with your credentials
    var config = {
        "product_name": "YourGameName",
        "product_version": "1.0.0",
        "product_id": "your_epic_product_id",
        "sandbox_id": "your_sandbox_id",
        "deployment_id": "your_deployment_id",
        "client_id": "your_client_id",
        "client_secret": "your_client_secret"
    }

    if EpicOS.initialize(config):
        print("✅ EOS initialized successfully")
        # EOS is now ready to use
    else:
        print("❌ Failed to initialize EOS")
        # Handle initialization failure

func _process(_delta: float) -> void:
    # The EpicOS singleton automatically ticks EOS every frame
    # No manual ticking required
    pass
```

**Note**: The `EpicOS` singleton is automatically registered as an autoload when the plugin is enabled. You don't need to manually add it to your project's autoloads.

## Configuration

### Getting Your EOS Credentials

1. Create an Epic Games Developer account at [dev.epicgames.com](https://dev.epicgames.com/)
2. Create a new product in the Developer Portal
3. Navigate to your product's settings to find:
   - **Product ID**: Your unique product identifier
   - **Sandbox ID**: Development environment identifier
   - **Deployment ID**: Deployment configuration ID
   - **Client ID & Secret**: OAuth credentials

### Configuration Options

The `EpicOS.initialize()` method accepts a dictionary with these options:

| Option | Type | Required | Description |
|--------|------|----------|-------------|
| `product_name` | String | No | Display name for your product (default: "GodotEOS") |
| `product_version` | String | No | Version string (default: "1.0.0") |
| `product_id` | String | **Yes** | Epic product ID from Developer Portal |
| `sandbox_id` | String | **Yes** | Sandbox ID for development |
| `deployment_id` | String | **Yes** | Deployment configuration ID |
| `client_id` | String | **Yes** | OAuth client ID |
| `client_secret` | String | **Yes** | OAuth client secret |
| `encryption_key` | String | No | Optional. If set, must be exactly 64 hexadecimal characters. |

## Usage Examples

### Basic EOS Integration

```gdscript
extends Node

func _ready():
    # Check if EOS is initialized
    if EpicOS.is_platform_initialized():
        print("EOS is ready!")

        # Your EOS-specific code here
        # - User authentication
        # - Friends management
        # - Achievements
        # - Leaderboards
        # - etc.
    else:
        print("EOS not initialized")
```

### Error Handling

```gdscript
func initialize_eos():
    var config = {
        "product_id": "your_product_id",
        "sandbox_id": "your_sandbox_id",
        "deployment_id": "your_deployment_id",
        "client_id": "your_client_id",
        "client_secret": "your_client_secret"
    }

    if not EpicOS.initialize(config):
        print("❌ EOS initialization failed!")
        # Handle initialization failure
        # - Show error message to user
        # - Disable online features
        # - Fall back to offline mode
        return false

    print("✅ EOS initialized successfully")
    return true
```

## Troubleshooting

### Common Issues

#### 1. Extension Not Loading
**Problem**: "GodotEOS" doesn't appear in plugins list

**Solutions**:
- Ensure the `.gdextension` file is in the correct location
- Check that binary files match your platform (Windows/Linux/macOS)
- Verify the extension file paths in `godoteos.gdextension`

#### 2. Missing EOS SDK Libraries
**Problem**: "Failed to load EOS SDK" or similar runtime errors

**Solutions**:
- Ensure EOS SDK `.dll`/`.so`/`.dylib` files are in your project's binary directory
- Check that you're using the correct EOS SDK version
- Verify library architecture matches (x64 vs x86)

#### 3. Compilation Errors
**Problem**: Build fails with missing headers or libraries

**Solutions**:
- Verify EOS SDK is properly extracted to `eos_sdk/` directory
- Check that `eos_sdk/Include/` contains header files
- Ensure `eos_sdk/Lib/` contains library files
- Try cleaning and rebuilding: `scons --clean && scons platform=windows`

#### 4. Platform Initialization Fails
**Problem**: `EpicOS.initialize()` returns `false`

**Solutions**:
- Verify all required credentials are provided
- Check Epic Developer Portal for correct IDs
- Ensure network connectivity
- Check console output for specific error messages

### Debug Logging

The extension includes detailed logging. Check Godot's output console for:
- `✅` Success messages
- `❌` Error messages
- `[EOS LOG]` EOS SDK log messages

### Getting Help

1. Check the [Epic Online Services Documentation](https://dev.epicgames.com/docs/epic-online-services)
2. Review Godot console output for error messages
3. Create an issue on the [GitHub repository](https://github.com/Flying-Rat/GodotEOS/issues)

## File Structure Reference

```
GodotEOS/
├── README.md
├── GUIDE.md                          # This file
├── LICENSE
├── project.godot                     # Main project file
├── eos_sdk/                          # EOS SDK (you provide)
│   ├── Bin/
│   │   ├── EOSSDK-Win64-Shipping.dll
│   │   ├── libEOSSDK-Linux-Shipping.so
│   │   └── libEOSSDK-Mac-Shipping.dylib
│   ├── Include/
│   │   ├── eos_sdk.h
│   │   └── [other headers...]
│   ├── Lib/
│   │   ├── EOSSDK-Win64-Shipping.lib
│   │   └── [other libraries...]
│   └── put_eos_sdk_here             # Marker file
├── godot_eos_extension/             # Extension source
│   ├── src/
│   │   ├── godotepic.h
│   │   ├── godotepic.cpp
│   │   └── [other source files...]
│   ├── SConstruct                    # Build configuration
│   ├── setup_gdextension.ps1         # Setup scripts
│   ├── setup_godot_cpp_module.ps1
│   ├── godot-cpp/                    # Godot C++ bindings
│   ├── doc_classes/
│   └── demo/                         # Build output and demo
│       ├── bin/                      # Compiled binaries
│       ├── godoteos.gdextension     # Extension manifest
│       ├── project.godot            # Demo project
│       ├── scenes/
│       └── scripts/
├── addons/
│   └── godoteos/                     # Plugin addon
│       ├── epic_os.gd               # EpicOS singleton
│       ├── plugin.cfg               # Plugin configuration
│       ├── plugin.gd                # Plugin script
│       └── bin/                     # Runtime binaries
├── scenes/                           # Demo scenes
├── scripts/                          # Demo scripts
├── screenshots/                      # Demo screenshots
└── docs/                            # Documentation
```

## Next Steps

1. **Authentication**: Implement user login with Epic accounts
2. **Friends System**: Add friends list and social features
3. **Achievements**: Set up achievement tracking
4. **Leaderboards**: Create competitive features
5. **Lobbies**: Add multiplayer lobby support

For advanced features, refer to the [Epic Online Services documentation](https://dev.epicgames.com/docs/epic-online-services) and extend the GodotEOS class with additional EOS SDK functionality.

---

**Happy coding! 🎮**