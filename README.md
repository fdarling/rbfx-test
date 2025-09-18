# Overview

FPS (first person "shooter") style game made using the 3D game engine "Urho3D", specifically its descendant / fork "rbfx". The project also compiles with the alternative fork "U3D".

# Dependencies

* [Rebel Fork Framework](https://github.com/rbfx/rbfx) aka "rbfx"
* [Open Asset Import Library](https://github.com/assimp/assimp) aka "assimp" (tested with v5.2.5)

## rbfx (Urho3D "Rebel Fork Framework")

```
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=~/apps/rbfx -DCMAKE_BUILD_TYPE=Release
#cmake .. -DCMAKE_INSTALL_PREFIX=~/apps/rbfx_debug -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Release --target install
#cmake --build . --config Debug --target install
```

# Compiling

```
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=~/apps/rbfx/share -DCMAKE_BUILD_TYPE=Release
#cmake .. -DCMAKE_PREFIX_PATH=~/apps/rbfx_debug/share -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

# Running

```
URHO3D_PREFIX_PATH=~/apps/rbfx/bin ./rbfx-test
```

# Controls

Keyboard hotkeys:

* <kbd>T</kbd> for cycle camera mode (free flying, first person, third person)
* <kbd>Z</kbd> to toggle graphics debug drawing
* <kbd>X</kbd> to toggle wireframe rendering mode
* <kbd>C</kbd> to toggle physics debug drawing
* <kbd>M</kbd> to toggle shadow mapping
* <kbd>O</kbd> to toggle SSAO (screen space ambient occlusion) shadows
* <kbd>TAB</kbd> to toggle mouse grabbing / mouse-look
* <kbd>ESC</kbd> to quit

Primary walking / flying controls (affects camera):

* <kbd>W</kbd> for walk/fly forward
* <kbd>S</kbd> for walk/fly backward
* <kbd>A</kbd> for walk/fly left
* <kbd>D</kbd> for walk/fly right
* <kbd>SPACE</kbd> for jump / vertical ascent
* <kbd>LCTRL</kbd> vertical descent

When the camera is in "fly" mode (detached from the player avatar), the following controls "remote control" the player avatar:

* <kbd>I</kbd> for walk forward
* <kbd>K</kbd> for walk backwards
* <kbd>J</kbd> for walk left
* <kbd>L</kbd> for walk right
* <kbd>RSHIFT</kbd> for jump

Mouse controls:

* when grabbing, mouselook using cursor motion
* left click shoots a ball

# Other Resources:

* [Urho3D](https://urho3d.io/) (project died, supposedly due to hostile takeover), GitHub archived repo: https://github.com/urho3d/urho3d
* [U3D](https://u3d.io/), GitHub repo: https://github.com/u3d-community/U3D
* [rbfx](https://rebelfork.io/) aka "Rebel Fork Framework", GitHub repo: https://github.com/rbfx/rbfx
* [Dry](https://dry.luckey.games/) aka "Dry Engine", GitLab repo: https://gitlab.com/luckeyproductions/dry
* Dviglo (Russian language fork by "1vanK"), GitHub repo: https://github.com/dviglo/dviglo
