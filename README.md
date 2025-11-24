# mc-afker

A Minecraft AFK bot built with Botcraft.

## Prerequisites

- CMake 3.19 or higher
- C++17 compatible compiler (GCC, Clang, or MSVC)
- Git (for submodule management)
- OpenSSL development libraries
  - Linux: `libssl-dev`
  - macOS: `openssl` (via Homebrew)
  - Windows: Included with Visual Studio

## Setup

### First Time Setup

Run the setup wizard to configure the bot:

```bash
./bin/mc-afker --setup
```

The setup wizard will ask for:
- Server address (e.g., `mc.example.com:25565`)
- Username (for offline mode)
- Password (for `/register` and `/login` commands)
- Warp locations and wait times

Configuration is saved to `config.json`. After setup, you can simply run:

```bash
./bin/mc-afker
```

### Configuration File

The bot uses `config.json` for configuration. Example:

```json
{
  "server": {
    "address": "mc.example.com:25565",
    "username": "YourBot",
    "password": "your_password"
  },
  "warps": [
    {
      "name": "creeper_farm",
      "wait_minutes": 30
    },
    {
      "name": "slime_farm",
      "wait_minutes": 30
    }
  ]
}
```

You can edit `config.json` directly or run `--setup` again to reconfigure.

## Building

### Quick Start

The easiest way to build the project is using the provided build scripts:

**Linux/macOS:**
```bash
./build.sh
```

**Windows:**
```cmd
build.bat
```

**Using Make (Linux/macOS):**
```bash
make
# Or with custom build type:
make BUILD_TYPE=Debug
```

### Manual Build

1. **Initialize submodules:**
   ```bash
   git submodule update --init --recursive
   ```

2. **Create build directory:**
   ```bash
   mkdir build
   cd build
   ```

3. **Configure CMake:**
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

4. **Build:**
   ```bash
   cmake --build . --config Release
   ```

The executable will be located at `build/bin/mc-afker` (or `build/bin/mc-afker.exe` on Windows).

### Build Options

You can configure Botcraft options through CMake:

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DBOTCRAFT_COMPRESSION=ON \
  -DBOTCRAFT_ENCRYPTION=ON \
  -DBOTCRAFT_GAME_VERSION=1.21.10
```

Available Botcraft options:
- `BOTCRAFT_COMPRESSION`: Enable compression (default: ON)
- `BOTCRAFT_ENCRYPTION`: Enable encryption/online mode (default: ON)
- `BOTCRAFT_GAME_VERSION`: Minecraft version to target (default: latest)
- `BOTCRAFT_USE_OPENGL_GUI`: Enable OpenGL GUI (default: OFF)
- `BOTCRAFT_USE_IMGUI`: Enable ImGui overlay (default: OFF)

## Docker Build

### Using Docker Compose

```bash
docker-compose up -d
docker-compose exec builder bash
```

Then inside the container:
```bash
./build.sh
```

### Using Dockerfile directly

```bash
docker build -t mc-afker .
docker run -it -v $(pwd):/workspace mc-afker
```

## CI/CD

The project includes GitHub Actions workflows for automated builds on:
- Linux (Ubuntu)
- macOS
- Windows

Builds are triggered on push to main/master/develop branches and on pull requests.

## Project Structure

```
mc-afker/
├── CMakeLists.txt          # Root CMake configuration
├── Makefile                # Make wrapper for CMake
├── build.sh                # Unix build script
├── build.bat               # Windows build script
├── Dockerfile              # Docker build environment
├── docker-compose.yml      # Docker Compose configuration
├── src/
│   └── main.cpp           # Main application source
└── libs/
    └── Botcraft/          # Botcraft library (git submodule)
```

## Usage

### Running the Bot

After setup, simply run:

```bash
./bin/mc-afker
```

The bot will:
1. Connect to the configured server
2. Register (first time) or login (subsequent runs)
3. Loop through configured warps, waiting the specified time at each

### Command Line Options

- `--setup` - Run setup wizard to configure the bot
- `--first-time` - Force registration (even if already registered)
- `--help` - Show help message

### Example Workflow

```bash
# First time: run setup
./bin/mc-afker --setup

# Subsequent runs: just run the bot
./bin/mc-afker

# Force re-registration
./bin/mc-afker --first-time
```

## Development

### Build Types

- `Debug`: Debug build with symbols
- `Release`: Optimized release build
- `RelWithDebInfo`: Release with debug info
- `MinSizeRel`: Minimum size release

### Cleaning Build

```bash
# Using Make
make clean

# Manual
rm -rf build
```

## License

This project uses the Botcraft library. See `libs/Botcraft/LICENSE` for Botcraft's license terms.

