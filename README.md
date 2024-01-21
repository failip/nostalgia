# Nostalgia

## Build

```bash
git submodule update --init --recursive
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DDEV_MODE=On -G Ninja
ninja
```