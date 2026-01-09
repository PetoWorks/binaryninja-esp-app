# Binary Ninja ESP APP Architecture Plugin

Espressif Application Format Loader

**Build (Linux)**
```
$ git clone https://github.com/PetoWorks/binaryninja-esp-app
$ git clone https://github.com/Vector35/binaryninja-api
$ cd binaryninja-api && cat <PATH_TO>/binaryninja/api_REVISION.txt
$ git checkout <REVISION>
$ git submodule update --init --recursive
$ cd ../binaryninja-esp-app
$ mkdir build
$ cmake -S . -B build -D BN_API_PATH="$(realpath ../binaryninja-api)" -D BN_INSTALL_DIR="<PATH_TO>/binaryninja"
$ cmake --build build
$ ln -sf "$(realpath ./build/libview_esp_app.so)" "$HOME/.binaryninja/plugins/libview_esp_app.so"

**Build (macOS)**
```
$ git clone https://github.com/PetoWorks/binaryninja-esp-app
$ git clone https://github.com/Vector35/binaryninja-api
$ cd binaryninja-esp-app
$ mkdir build
$ cmake -S build -B build -D BN_API_PATH="$(realpath ../binaryninja-api)"
$ cmake --build build
$ ln -s "$(realpath ./build/libview_esp_app.dylib)" "$HOME/Library/Application Support/Binary Ninja/plugins/libview_esp_app.dylib"
```

**Build (Windows)**
```
$ git clone https://github.com/PetoWorks/binaryninja-esp-app
$ git clone https://github.com/Vector35/binaryninja-api
$ cd binaryninja-esp-app
$ mkdir build
$ cmake -A x64 -S . -B build -D BN_API_PATH="$PWD\..\binaryninja-api"
$ cmake --build build --config Release
$ ls build\Release\view_esp_app.dll
```

Big thanks to @emesare to help write this plugin
