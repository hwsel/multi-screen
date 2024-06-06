# multi-screen

## Prepare Project

```
cmake -DEP_BUILD_ALWAYS=<force_build_externals> -DEP_J=<max_jobs> -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=<ninja_path> -G Ninja -S <project_root> -B <project_root>/cmake-build-debug
```

`<project_root>` is where your project located. `<ninja_path>` is where your `ninja` program located or installed, for example, `/usr/bin/ninja`. `<max_jobs>` is the `-j` option in `make` and `ninja` depends on your CPU capacity, for example, set to 36 on a 36 cores CPU and set to 3 on a raspberry pi. `<force_build_externals>` is the option if you want to force the build process for those externals, set to `1L` to build them again.

For example, on a 36 cores CPU, `/cmake -DEP_BUILD_ALWAYS=1L -DEP_J=36 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=/usr/bin/ninja -G Ninja -S . -B ./cmake-build-debug`

```
cmake --build <project_root>/cmake-build-debug --clean-first --target player -j 3
```

