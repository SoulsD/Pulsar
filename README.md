# Pulsar
Pusar Engine - Based on Vulkan

- [Pulsar](#pulsar)
- [Build](#build)
    - [Windows / MSYS2 (Mingw64)](#windows--msys2-mingw64)
- [VS Code configuration](#vs-code-configuration)
    - [Windows / MSYS2](#windows--msys2)
        - [Git](#git)
        - [C/Cpp IntelliSense](#ccpp-intellisense)
        - [Code formatting - clang-format](#code-formatting---clang-format)
        - [Integrated Terminal](#integrated-terminal)
        - [Tasks](#tasks)
        - [Debugger - gdb](#debugger---gdb)

# Build
TODO

## Windows / MSYS2 (Mingw64)
https://www.msys2.org/

```sh
pacman -S mingw64/mingw-w64-x86_64-{gcc,vulkan,glm,glfw,glslang,gtest} make
# optional : mingw64/mingw-w64-x86_64-{vulkan-docs,spirv-tools}
# optional : mingw64/mingw-w64-x86_64-{vulkan-man-pages}

# pacman -S mingw64/mingw-w64-x86_64-{cmake,spirv-tools} base-devel #contains man,gdb?,make
# C:/msys64/usr/bin/cp.exe => copy.exe
# vk_loader_platform.h -> #define _WIN32_WINNT 0x0600
# cmake .. -G"Unix Makefiles" -DGLSLANG_INSTALL_DIR=/mingw64/bin/ -DVULKAN_HEADERS_INSTALL_DIR=/home/apele/projects/Vulkan-Headers/build/install/ #"MSYS Makefiles"
```

Add the Vulkan layers path to your environment (mandatory for debug build) :
```sh
export VK_LAYER_PATH=/mingw64/lib/
```

# VS Code configuration

## Windows / MSYS2
https://www.msys2.org/


### Git
https://git-scm.com/ *

\* git package for msys2 is known to not working well under VS Code


### C/Cpp IntelliSense
Update configuration in file `.vscode/c_cpp_properties.json` with the the following:

```json
"compilerPath": "C:/msys64/mingw64/bin/gcc",
"defines": [
    // ...
    "__GNUC__=7",
    "__cdecl=__attribute__((__cdecl__))"
]
```

Moreover, in file `.vscode/settings.json` you can add :
```json
"C_Cpp.intelliSenseEngine": "Default",
```


### Code formatting - clang-format

```sh
pacman -S mingw64/mingw-w64-x86_64-clang
```

In file `.vscode/settings.json` you can then add :
```json
"editor.formatOnSave": true,
"C_Cpp.clang_format_path": "C:\\msys64\\mingw64\\bin\\clang-format.exe",
```


### Integrated Terminal
In file `.vscode/settings.json` :
```json
"terminal.integrated.shell.windows": "C:\\msys64\\usr\\bin\\bash.exe",
"terminal.integrated.shellArgs.windows": [
    "--login"
],
"terminal.integrated.env.windows": {
    "CHERE_INVOKING": "1",
    "MSYSTEM": "MINGW64"
}
```


### Tasks
TODO

### Debugger - gdb
Install package `mingw64/mingw-w64-x86_64-gdb` then modify the default configuration of your `.vscode/launch.json`

```json
"windows": {
    "miDebuggerPath": "C:/msys64/mingw64/bin/gdb.exe"
}
```

Don't forget to build in debug mode before lauching : 
```sh
make dbg
```

You can define multiple configurations to launch the unit tests and the main program :
```json
"windows": {
    "program": "${workspaceFolder}/test/run_unittest.exe"
}
```
or
```json
"windows": {
    "program": "${workspaceFolder}/buid/pulsar.exe"
}
```
