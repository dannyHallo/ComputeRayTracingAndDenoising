# Vulkan Compute Shader Based Ray Tracer

This ray tracer is loosely based on the [Ray Tracing in One Weekend series](https://raytracing.github.io), adapted for real-time rendering on GPU, and with some extra features like temporal accumulation, low discrepancy noise, and SVGF filtering.

# Roadmap

1. memory barrier structure & forwarding pairs
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

# Directory Architecture

The `src` folder contains the source code for the Vulkan Compute Shader Based Ray Tracer project. Here's a breakdown of the directory structure:

## app-context

This directory contains the implementation of the VulkanApplicationContext class, which is responsible for creating and managing the Vulkan instance, device, and swapchain.

- `VulkanApplicationContext.cpp`: Implementation of the VulkanApplicationContext class.
- `VulkanApplicationContext.h`: Header file for the VulkanApplicationContext class.

## memory

This directory contains utility classes for managing Vulkan memory resources.

- `Buffer.h`: Header file for the Buffer class, which represents a Vulkan buffer object.
- `Image.cpp`: Implementation of the Image class, which represents a Vulkan image object.
- `Image.h`: Header file for the Image class.
- `ImageUtils.cpp`: Implementation of utility functions for working with Vulkan images.
- `ImageUtils.h`: Header file for the ImageUtils utility functions.

## ray-tracing

This directory contains the implementation of the ray tracing algorithm.

- `Bvh.cpp`: Implementation of the Bounding Volume Hierarchy acceleration structure.
- `Bvh.h`: Header file for the Bounding Volume Hierarchy acceleration structure.
- `GpuModels.h`: Header file containing the vertex and index data for the GPU models used in the scene.
- `RtScene.cpp`: Implementation of the Ray Tracing Scene class, which manages the scene data and performs ray tracing.
- `RtScene.h`: Header file for the Ray Tracing Scene class.

## render-context

This directory contains the implementation of the Vulkan render context.

- `FlatRenderPass.cpp`: Implementation of the FlatRenderPass class, which renders the scene to a flat image.
- `FlatRenderPass.h`: Header file for the FlatRenderPass class.
- `ForwardRenderPass.cpp`: Implementation of the ForwardRenderPass class, which renders the scene using forward rendering.
- `ForwardRenderPass.h`: Header file for the ForwardRenderPass class.
- `RenderPass.h`: Header file for the RenderPass base class.
- `RenderSystem.cpp`: Implementation of the RenderSystem class, which manages the Vulkan render context.
- `RenderSystem.h`: Header file for the RenderSystem class.

## scene

This directory contains the implementation of the scene graph.

- `ComputeMaterial.cpp`: Implementation of the ComputeMaterial class, which represents a material that is computed on the GPU.
- `ComputeMaterial.h`: Header file for the ComputeMaterial class.
- `ComputeModel.cpp`: Implementation of the ComputeModel class, which represents a model that is computed on the GPU.
- `ComputeModel.h`: Header file for the ComputeModel class.
- `DrawableModel.cpp`: Implementation of the DrawableModel class, which represents a model that is drawn using the GPU.
- `DrawableModel.h`: Header file for the DrawableModel class.
- `Material.cpp`: Implementation of the Material class, which represents a material that is drawn using the GPU.
- `Material.h`: Header file for the Material class.
- `mesh.cpp`: Implementation of the Mesh class, which represents a mesh of vertices and indices.
- `mesh.h`: Header file for the Mesh class.
- `Scene.cpp`: Implementation of the Scene class, which represents the scene graph.
- `Scene.h`: Header file for the Scene class.

## utils

This directory contains utility classes and functions.

- `Camera.cpp`: Implementation of the Camera class, which represents a camera in the scene.
- `Camera.h`: Header file for the Camera class.
- `glm.h`: Header file for the GLM math library.
- `readfile.h`: Header file for the readfile utility function, which reads a file into a string.
- `RootDir.h`: Header file for the RootDir constant, which contains the root directory of the project.
- `RootDir.h.in`: Input file for the RootDir.h header file.
- `StbImageImpl.cpp`: Implementation of the stb_image library.
- `StbImageImpl.h`: Header file for the stb_image library.
- `systemLog.h`: Header file for the systemLog utility function, which logs messages to the console.
- `vulkan.h`: Header file for the Vulkan API.
- `Window.cpp`: Implementation of the Window class, which represents the application window.
- `Window.h`: Header file for the Window class.

## References

- https://alain.xyz/blog/ray-tracing-filtering
- http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences
- https://psychopath.io/post/2014_06_28_low_discrepancy_sequences
