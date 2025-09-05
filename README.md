# Dependencies

## rbfx (Urho3D "Rebel Fork Framework")

```
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=~/apps/rbfx
cmake --build . --config Release --target install
#cmake --build . --config Debug --target install
```

# Compiling

```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=~/apps/rbfx/share
#cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=~/apps/rbfx/share
cmake --build .
```

# Running

```
URHO3D_PREFIX_PATH=~/apps/rbfx/bin ./rbfx-test
```

# Other Resources:

* [Urho3D](https://urho3d.io/) (project died, supposedly due to hostile takeover), GitHub archived repo: https://github.com/urho3d/urho3d
* [U3D](https://u3d.io/), GitHub repo: https://github.com/u3d-community/U3D
* [rbfx](https://rebelfork.io/) aka "Rebel Fork Framework", GitHub repo: https://github.com/rbfx/rbfx
* [Dry](https://dry.luckey.games/) aka "Dry Engine", GitLab repo: https://gitlab.com/luckeyproductions/dry
