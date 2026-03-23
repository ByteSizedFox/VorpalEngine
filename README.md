# VorpalEngine
Desktop + VR game engine

code added past 2025 may include AI assisted development

# Building
Build Dependencies (fedora tested only)
```bash
sudo dnf install cmake make gcc g++ wayland-devel libxkbcommon-devel libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel mesa-libGL-devel vulkan-validation-layers
```

To build, use cmake
```bash
mkdir build
cd build
cmake ..
make
```

when running the engine, call vorpal engine from inside the assets folder
```bash
cd assets
../build/vorpal_engine
```
