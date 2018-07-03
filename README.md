# Pulsar
Pusar Engine - Based on Vulkan

- [Pulsar](#pulsar)
- [Build](#build)
- [VS Code configuration](#vs-code-configuration)
    - [Windows / MSYS2](#windows---msys2)
        - [Git](#git)
        - [C/Cpp IntelliSense](#c-cpp-intellisense)
        - [Integrated Terminal](#integrated-terminal)
        - [Tasks](#tasks)
        - [Debugger](#debugger)

# Build
TODO

## Windows / MSYS2
https://www.msys2.org/

```sh
pacman -S mingw64/mingw-w64-x86_64-{gcc,make,vulkan-devel,glm,glfw}
```

# VS Code configuration

## Windows / MSYS2
https://www.msys2.org/

### Git
https://git-scm.com/ *

\* git package for msys2 is known to not working well under VS Code


### C/Cpp IntelliSense
Update `includePath` in file `.vscode/c_cpp_properties.json` with the outputs of the following command :

```sh
echo | gcc -Wp,-v -x c++ - -fsyntax-only
```

and add the following `defines` :
```json
"__GNUC__=7",
"__cdecl=__attribute__((__cdecl__))"
```

Furthermore, in file `.vscode/settings.json` you can add :
```json
"C_Cpp.intelliSenseEngine": "Default",
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
Install package `mingw64/mingw-w64-x86_64-gdb` then add the following configuration to your launch.json
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
