# 基于 Vulkan 计算着色器的光线追踪器

[英文版本说明](./README.md)

这个光线追踪器基于[Ray Tracing in One Weekend 系列](https://raytracing.github.io)，适用于 GPU 实时渲染，并具有一些额外的功能，如时间累积、低差异噪声和 SVGF 滤波。

# 截图

| 图像类型     | 图像                                  |
| ------------ | ------------------------------------- |
| 原始图像     | ![Raw](./imgs/1.%20raw.png)           |
| 时间累积图像 | ![Temporal](./imgs/2.%20temporal.png) |
| 滤波图像     | ![Filtered](./imgs/3.%20filtered.png) |

# 特点

- 时间样本累积
- 低差异噪声的实现
- A-Trous 去噪器
- SVGF 去噪器（无方差）

# 待办事项

- [ ] 硬件加速光线追踪
- [ ] 稀疏体素八叉树

# 如何运行

只需运行路径为`./Release/vulkan.exe`的预编译版本，无需任何预设参数。

# 如何编译和运行

这些说明仅适用于 Windows 平台，但可以通过一些微小的修改在 Linux 上工作。

1. 运行`compile_shader_cpp.bat`。这个 shell 脚本将编译所有的着色器并运行 CMake 以获取可执行文件。
2. 运行路径为`./build/Release/vulkan.exe`的可执行文件。

# 硬件要求

这个程序可以在支持 Vulkan 的现代 GPU 上工作，因为它只使用了计算着色器实用程序。

通常，您需要一张支持 Vulkan 1.2 和 SPIR-V 1.5 的 GPU。您可以在[这里](https://vulkan.gpuinfo.org/)检查您的 GPU 对 Vulkan 的支持。

# 用户控制

用户可以使用鼠标作为输入来控制相机的滚动和旋转角度。此外，相机的移动可以使用以下键来控制：

- **W**：向前移动相机
- **A**：向左移动相机
- **S**：向后移动相机
- **D**：向右移动相机
- **SPACE**：向上移动相机
- **SHIFT**：向下移动相机

要解锁鼠标并控制终端，用户可以按**TAB**键。这使用户能够在运行时自由选择去噪技术。要再次锁定鼠标，用户可以再次按**TAB**键。

要中止程序，用户可以按**ESC**键。

# 用户指南

这个项目是各种去噪技术的最先进实现，用于实时渲染。它具有**时间样本累积**、**低差异噪声**、**A-Trous 去噪器**和**SVGF 去噪器 (无方差)** 等一些额外的功能。所有这些技术都是使用基本的计算着色器实现的，这使它们易于实现和跨平台。

**时间样本累积**是一种通过随时间累积多个样本来减少实时渲染中噪声的技术。这种技术特别适用于减少动画和其他动态场景中的噪声。

**低差异噪声**是一种噪声类型，旨在比传统随机噪声更具视觉吸引力。它通过以更均匀和可预测的方式分布噪声来实现这一目标，从而减少可见的伪影和更平滑的外观。

**A-Trous 去噪器**是一种在实时渲染中去噪图像的流行技术。它通过对图像应用一系列具有不同内核大小的滤波器来实现。这使它能够在保留细节和纹理的同时去除噪声。
**SVGF 去噪器**是另一种在实时渲染中去噪图像的流行技术。它通过估计图像的基础信号并过滤噪声来工作。这种技术特别适用于去除高频噪声，同时保留锐利的边缘和细节。

总的来说，这个项目提供了一套全面的去噪技术，可以用于改善实时渲染应用程序的视觉质量。无论您是在制作游戏、模拟还是任何其他类型的实时应用程序，这些技术都可以帮助您实现更平滑、更具视觉吸引力的结果。而且，因为它们是使用计算着色器实现的，所以它们易于使用和跨平台。

# 目录结构

`src`文件夹包含了 Vulkan Compute Shader Based Ray Tracer 项目的源代码。以下是目录结构的详细说明：

## app-context

这个目录包含了 VulkanApplicationContext 类的实现，该类负责创建和管理 Vulkan 实例、设备和交换链。

- `VulkanApplicationContext.cpp`：VulkanApplicationContext 类的实现。
- `VulkanApplicationContext.h`：VulkanApplicationContext 类的头文件。

## memory

这个目录包含了用于管理 Vulkan 内存资源的实用程序类。

- `Buffer.h`：Buffer 类的头文件，表示 Vulkan 缓冲区对象。
- `Image.cpp`：Image 类的实现，表示 Vulkan 图像对象。
- `Image.h`：Image 类的头文件。
- `ImageUtils.cpp`：用于处理 Vulkan 图像的实用程序函数的实现。
- `ImageUtils.h`：ImageUtils 实用程序函数的头文件。

## ray-tracing

这个目录包含了光线追踪算法的实现。

- `Bvh.cpp`：Bounding Volume Hierarchy 加速结构的实现。
- `Bvh.h`：Bounding Volume Hierarchy 加速结构的头文件。
- `GpuModels.h`：包含场景中使用的 GPU 模型的顶点和索引数据的头文件。
- `RtScene.cpp`：Ray Tracing Scene 类的实现，管理场景数据并执行光线追踪。
- `RtScene.h`：Ray Tracing Scene 类的头文件。

## render-context

这个目录包含了 Vulkan 渲染上下文的实现。

- `FlatRenderPass.cpp`：FlatRenderPass 类的实现，将场景渲染为平面图像。
- `FlatRenderPass.h`：FlatRenderPass 类的头文件。
- `ForwardRenderPass.cpp`：ForwardRenderPass 类的实现，使用前向渲染渲染场景。
- `ForwardRenderPass.h`：ForwardRenderPass 类的头文件。
- `RenderPass.h`：RenderPass 基类的头文件。
- `RenderSystem.cpp`：RenderSystem 类的实现，管理 Vulkan 渲染上下文。
- `RenderSystem.h`：RenderSystem 类的头文件。

## scene

这个目录包含了场景图的实现。

- `ComputeMaterial.cpp`：ComputeMaterial 类的实现，表示在 GPU 上计算的材质。
- `ComputeMaterial.h`：ComputeMaterial 类的头文件。
- `ComputeModel.cpp`：ComputeModel 类的实现，表示在 GPU 上计算的模型。
- `ComputeModel.h`：ComputeModel 类的头文件。
- `DrawableModel.cpp`：DrawableModel 类的实现，表示使用 GPU 绘制的模型。
- `DrawableModel.h`：DrawableModel 类的头文件。
- `Material.cpp`：Material 类的实现，表示使用 GPU 绘制的材质。
- `Material.h`：Material 类的头文件。
- `mesh.cpp`：Mesh 类的实现，表示顶点和索引的网格。
- `mesh.h`：Mesh 类的头文件。
- `Scene.cpp`：Scene 类的实现，表示场景图。
- `Scene.h`：Scene 类的头文件。

## utils

这个目录包含了实用程序类和函数。

- `Camera.cpp`：Camera 类的实现，表示场景中的相机。
- `Camera.h`：Camera 类的头文件。
- `glm.h`：GLM 数学库的头文件。
- `readfile.h`：readfile 实用程序函数的头文件，将文件读入字符串中。
- `RootDir.h`：RootDir 常量的头文件，包含项目的根目录。
- `RootDir.h.in`：RootDir.h 头文件的输入文件。
- `StbImageImpl.cpp`：stb_image 库的实现。
- `StbImageImpl.h`：stb_image 库的头文件。
- `systemLog.h`：systemLog 实用程序函数的头文件，将消息记录到控制台。
- `vulkan.h`：Vulkan API 的头文件。
- `Window.cpp`：Window 类的实现，表示应用程序窗口。
- `Window.h`：Window 类的头文件。

## 参考文献

- https://alain.xyz/blog/ray-tracing-filtering
- http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences
- https://psychopath.io/post/2014_06_28_low_discrepancy_sequences
