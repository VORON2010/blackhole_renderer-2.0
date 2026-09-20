# Black Hole Renderer 2.0

A highly realistic, real-time black hole rendering engine written in C++ and GLSL. This project leverages Raymarching and the Kerr-Schild metric to simulate the gravitational lensing of a spinning black hole, complete with an accretion disk and relativistic jets.

## Showcase

*(Run `convert_to_gif.bat` after recording videos with F5 to generate these GIFs!)*

<p align="center">
  <img src="media/gif1.gif" width="45%" alt="Black Hole Demo 1" />
  <img src="media/gif2.gif" width="45%" alt="Black Hole Demo 2" />
</p>
<p align="center">
  <img src="media/gif3.gif" width="45%" alt="Cinematic Flight" />
  <img src="media/gif4.gif" width="45%" alt="Relativistic Jets" />
</p>

## Features

- **Relativistic Raymarching**: Real-time integration of light paths through the curved spacetime of a Kerr black hole using Runge-Kutta integration.
- **Accretion Disk & Jets**: Fully customizable accretion disk and relativistic jets with doppler beaming, gravitational redshift, and temperature mapping.
- **Cinematic Keyframing**: Built-in cinematic camera system. Press `F6` to add keyframes and `F7` to play them back smoothly.
- **Video Recording**: Press `F5` to start recording the output directly to `.mp4` (requires `ffmpeg` in your PATH).
- **Extensive UI Controls**: Real-time adjustment of black hole mass, spin, charge, disk brightness, and more via an ImGui overlay.

## Requirements

- CMake 3.20 or higher
- C++17 compatible compiler (MSVC, GCC, Clang)
- OpenGL 3.3+ capable GPU
- `ffmpeg` (optional, for video recording)

## Build Instructions

1. Clone the repository:
   ```bash
   git clone <your_repo_url>
   cd megarealstic_clion
   ```
2. Build with CMake:
   ```bash
   mkdir cmake-build-release && cd cmake-build-release
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build . --config Release
   ```
3. Run the executable `megarealstic` (or `megarealstic.exe` on Windows).

## Controls

- **WASD / Mouse**: Move and rotate the camera
- **Scroll Wheel**: Change movement speed
- **F5**: Toggle video recording
- **F6**: Add a cinematic keyframe
- **F7**: Play cinematic camera path
- **F12**: Take a screenshot
- **Space**: Toggle UI

## License

This project is open-source. See the repository for details.
