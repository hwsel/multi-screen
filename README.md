# multi-screen

## Prepare Project

```
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=<ninja_path> -G Ninja -S <project_root> -B <project_root>/cmake-build-debug
```

```
cmake --build <project_root>/cmake-build-debug --clean-first --target player -j 3
```

