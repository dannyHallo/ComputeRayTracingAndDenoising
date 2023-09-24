# Vulkan Compute Shader Based Ray Tracer

This ray tracer is loosely based on the [Ray Tracing in One Weekend series](https://raytracing.github.io), adapted for real-time rendering on GPU, and with some extra features like temporal accumulation, low discrepancy noise, and SVGF filtering.

# Roadmap

2. add normal and depth edge stopping functions to temporal accumulation
3. tap filter to temporal accumulation

# Design Decisions

1. Separable kernels for A-Trous denoiser is just not correct. And the performance gain is subtle regarding to a 3x3 kernel. So I decided to use the non-separable version.

# Screenshots

| Image Type | Image                                 |
| ---------- | ------------------------------------- |
| Raw        | ![Raw](./imgs/1.%20raw.png)           |
| Temporal   | ![Temporal](./imgs/2.%20temporal.png) |
| Filtered   | ![Filtered](./imgs/3.%20filtered.png) |

# Features

- Temporal sample accumulation
- Implementation of low discrepancy noise
- A-Trous denoiser
- SVGF denoiser (without variance)

# Todos

- [ ] Hardware-accelerated ray tracing
- [ ] Sparse Voxel Octree

# How to run

Simply run the pre-compiled version at path `./Release/vulkan.exe`, with no parameters needed.

# Guide for vscode users

This project uses clangd as the language server, and clang-tidy as the linter. To use this project in vscode, you need to install the following extensions:
LLVM: includes clangd and clang-tidy

# Hardware Requirements

This program can be worked on every modern GPUs that support Vulkan, since it only utilizes the compute shader utility.

Typically, you need a GPU that supports Vulkan 1.2 and SPIR-V 1.5. You can check your GPU's support for Vulkan [here](https://vulkan.gpuinfo.org/).

# User Control

The user can control the camera scroll and roll angle by using the mouse as input. Additionally, the movement of the camera can be controlled using the following keys:

- **W**: Move the camera forward
- **A**: Move the camera left
- **S**: Move the camera backward
- **D**: Move the camera right
- **SPACE**: Move the camera up
- **SHIFT**: Move the camera down

To unlock the mouse and control the terminal, the user can press the **TAB** key. This gives the user the ability to select the denoising techniques freely at run time. To lock the mouse again, the user can press **TAB** again.

To terminate the program, the user can press the **ESC** key.

# User Guide

This project is a state-of-the-art implementation of various denoising techniques for real-time rendering. It features **temporal sample accumulation**, **low discrepancy noise**, **A-Trous denoiser**, and **SVGF denoiser (without variance)**. All of these techniques are implemented using basic compute shaders, which makes them easy to implement and cross-platform.

**Temporal sample accumulation** is a technique that reduces noise in real-time rendering by accumulating multiple samples over time. This technique is particularly useful for reducing noise in animations and other dynamic scenes.

**Low discrepancy noise** is a type of noise that is designed to be more visually pleasing than traditional random noise. It achieves this by distributing the noise in a more uniform and predictable way, resulting in less visible artifacts and a smoother appearance.

The **A-Trous denoiser** is a popular technique for denoising images in real-time rendering. It works by applying a series of filters to the image, each with a different kernel size. This allows it to remove noise while preserving fine details and textures.

The **SVGF denoiser** is another popular technique for denoising images in real-time rendering. It works by estimating the underlying signal of the image and filtering out the noise. This technique is particularly effective at removing high-frequency noise while preserving sharp edges and details.

Overall, this project provides a comprehensive set of denoising techniques that can be used to improve the visual quality of real-time rendering applications. Whether you're working on a game, a simulation, or any other type of real-time application, these techniques can help you achieve a smoother, more visually pleasing result. And because they are implemented using compute shaders, they are easy to use and cross-platform.

## References

- https://alain.xyz/blog/ray-tracing-filtering
- http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences
- https://psychopath.io/post/2014_06_28_low_discrepancy_sequences
