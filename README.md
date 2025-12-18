# Binary Ninja ESP APP Architecture Plugin

Espressif Application Format Loader

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
$ cmake -G "Visual Studio 18 2026" -A x64 -S . -B build -D BN_API_PATH="$PWD\..\binaryninja-api"
$ cmake --build build --config Release
$ ls build\Release\view_esp_app.dll
```

Big thanks to @emesare to help write this plugin
