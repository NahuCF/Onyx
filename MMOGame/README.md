# MMO Game - Onyx Engine Example

This is a simple game project that demonstrates how to use the Onyx engine to render quads.

## What it does

- Initializes the Onyx engine with a window (1024x768)
- Renders three colored quads on screen
- Shows an ImGui debug window with FPS counter
- Allows changing the background color

## How to Build and Run

### Prerequisites

First, make sure you have the required packages installed:

```bash
# Install CMake and build tools
sudo apt-get update
sudo apt-get install -y cmake build-essential

# Install required graphics libraries
sudo apt-get install -y \
    libgl1-mesa-dev \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libxxf86vm-dev
```

### Building the Game

From the root Onyx directory:

```bash
# Method 1: Using the build script (easiest)
./build.sh Debug

# Method 2: Using CMake directly
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# Method 3: Build only MMOGame (faster)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target MMOGame -j$(nproc)
```

### Running the Game

After building successfully:

```bash
# Run the game
./build/bin/MMOGame

# Or if using Debug configuration
./build/Debug/bin/MMOGame
```

### What You Should See

- A window titled "MMO Game"
- Three colored quads:
  - Cyan quad in the center
  - Magenta quad on the right
  - Yellow quad on the left
- An ImGui debug window showing:
  - Current FPS
  - Frame time in milliseconds
  - Background color picker

### Troubleshooting

1. **CMake not found**: Install CMake with `sudo apt-get install cmake`

2. **OpenGL libraries missing**: Install the graphics libraries listed in prerequisites

3. **Build errors about missing headers**: The CMake system will automatically download dependencies. If it fails, try:
   ```bash
   # Clean build
   rm -rf build
   cmake -B build -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```

4. **Runtime error - can't open display**: Make sure you're running in a graphical environment (not SSH without X forwarding)

5. **Black window**: This might be a graphics driver issue. Update your graphics drivers.

## Code Structure

The `Main.cpp` file contains:

1. **GameLayer class**: Handles rendering and updates
   - `OnUpdate()`: Called every frame to render quads
   - `OnImGui()`: Creates the debug UI

2. **MMOGameApplication class**: Main application class
   - Sets up window properties
   - Creates and manages the game layer

3. **CreateApplication()**: Entry point function required by Onyx engine