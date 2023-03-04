# Vulkan compute shader based ray tracer.

<img width="500" alt="Screen Shot 2021-08-01 at 18 36 16" src="https://user-images.githubusercontent.com/44236259/127766493-e2402bde-48ca-462a-8110-d849151e9d18.png">

Ray tracer loosely based on [raytracing in one weekend series](https://raytracing.github.io), adapted for real time rendering on GPU.

## Accomplished:

- Temporal Accumulation
- Low Discrepancy Noise
- A-Trous
- SVGF (but without variance calc)

## TODOs:

- [ ] Vulkan hardware accelerated ray tracing
- [ ] Sparse voxel octree

## References

https://alain.xyz/blog/ray-tracing-filtering
http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences
https://psychopath.io/post/2014_06_28_low_discrepancy_sequences

## How to run

This is an instruction for mac os, but it should work for other systems too, since all the dependencies come from git submodules and build with cmake.

1. Download and install [Vulkan SDK] (https://vulkan.lunarg.com). Add $VULKAN_SDK environmental variable.
2. Pull glfw, glm, stb and obj loader:

```
git submodule init
git submodule update
```

3. Create a buld folder and step into it.

```
mkdir build
cd build
```

4. Run cmake. It will create `makefile` in build folder.

```
cmake -S ../ -B ./
```

5. Create an executable with makefile.

```
make
```

6. Compile shaders. You might want to run this with sudo if you dont have permissions for write.

```
mkdir ../resources/shaders/generated
sh ../compile.sh
```

7. Run the executable.

```
./vulkan
```
