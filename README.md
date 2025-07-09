<div align="center" class="grid cards" style="display: flex; align-items: center; justify-content: center; text-decoration: none;" markdown>
    <a href="https://www.linkedin.com/in/minhduong-thechosenone/">
        <img src="assets/App/AstrocelerateLogo-Branded.png" alt="Logo" width="75%">
    </a>
    <p>Copyright © 2024-2025 Dương Duy Nhật Minh, D.B.A. Oriviet Aerospace. All Rights Reserved.</p>
    </br>
</div>



# Table of Contents
* [About Astrocelerate](#about-astrocelerate)
    * [Goal](#goal)
    * [Vision](#vision)
        * [Short-term vision (MVP)](#short-term-vision-mvp)
    * [Design philosophy](#design-philosophy)
* [Features](#features)
    * [Currently implemented](#currently-implemented)
    * [Near-future plans](#near-future-plans)
* [Installation](#installation)
    * [Prerequisites](#prerequisites)
    * [Bootstrapping](#bootstrapping)
* [History](#history)
    * [Screenshots](#screenshots)

---

# About Astrocelerate
Astrocelerate is a spaceflight simulation engine tailored to space missions, developed in C++ with the Vulkan graphics API. 

## Goal
Astrocelerate aims to provide high-fidelity real-time and interactive visualizations with the accuracy of CPU-accelerated physics, the visual appeal and efficiency of GPU-accelerated rendering and parallelizable physics computing, and support for on-the-fly variable changes with real-time visual feedback.

Astrocelerate also aims to incorporate modern design principles, standards, and GUI into the workflow, making it appealing for both professional researchers and students alike.

## Vision
### Short-term vision (MVP)
The short term vision of Astrocelerate is to create a Minimum-Viable Product (MVP) that satisfies core features. Specifically, the MVP must be able to achieve these minima:
- Accurate real-time and interactive visualizations demonstrating simple scenarios (e.g., Earth-satellite orbit, basic maneuver simulation, the "launch, enter orbit, stage separation, re-entry, recovery" sequence)
- A clean, polished GUI (planned to be made with *Dear ImGui*)
- An Entity-Component-System (ECS)-based design

## Design philosophy
Astrocelerate's design philosophy takes inspiration from that of the Unity game engine and the GMAT spaceflight simulation engine.
These principles are subject to change over time, as Astrocelerate (and I) become more mature.

|              Core design principles              | Description                                                                                                                                                                                                                                                                                                                                                                                                                    |
| :----------------------------------------------: | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
|         **Open-source**          | Astrocelerate aims to be transparent and thrives on community feedback, modification, and extension. Therefore, to maximize contribution and growth, it is decided that the engine should be open-source.                                                                                                                                                              |
| **ECS-based, Modular & Extensible Architecture** | Astrocelerate is structured to allow flexibility and modularity, allowing users to set up and develop their own environments, objects, gravity, etc. with built-in, third-party, and custom "modules". This is part of Astrocelerate's integration of Entity-Component-System principles.<br><br>Astrocelerate is built to allow third-party plug-ins and extensions, enabling researchers to integrate new models or solvers. |
|      **Non-proprietary scripting language**      | Astrocelerate allows users to program their own modules and override built-in counterparts with C++.                                                                                                                                                                                                                                                                                                                           |
|          **Support for real missions**           | Astrocelerate can be used for operational mission planning (e.g., lunar and interplanetary transfers, spacecraft launch-and-recovery procedures).                                                                                                                                                                                                                                                                              |


# Features
## Currently implemented
- Real-time orbital propagation
- Accurate 2-body simulation
- Configurable physics time step
- Reference frame system with fixed simulation scaling (maintains physical accuracy across polarized scales)
- Custom ECS-based architecture with sparse-set storage (allows for performant simulation and efficient data flow between the GUI and backend)
- Live telemetry data, console, etc.; Tweakable simulation settings
- Offscreen rendering (allows for custom viewports, post-processing, etc.)

## Near-future plans
- Swappable numerical integrators (Symplectic Euler, RK4)
- Data-caching in ECS for even more performance
- Compute shaders and the offloading of parallelizable processes to the GPU
- Dynamic scaling for seamless transitions (e.g., from terrain to planet)
- N-body simulation capabilities
- A more diverse range of numerical integration methods (Verlet, Simpson's Rule, Gaussian quadrature)
- Solvers, coordinate systems, epochs
- C++ scripting to allow for user-created simulations
- Serialization of GUI and simulation data, with basic exporting capabilities
- A more advanced model loader, with better, correctly mapped textures and visual fidelity


# Installation
> [!WARNING]
> Astrocelerate has only been tested on Windows, although the application aims to be cross-platform-compatible.

## Prerequisites
- Vulkan SDK (Vulkan 1.2+)
- Vcpkg dependency manager
- Python 3.9+
- C++20

## Bootstrapping
- Run `SetupDebug.*` to set up a Debug configuration, or `SetupRelease.*` to set up a Release configuration, according to your operating system.
- Alternatively, you can manually run `GenerateDirectories.bat` to ensure file discovery is up-to-date, then run `scripts/Bootstrap.py` and follow on-screen instructions.


# History
Astrocelerate made its first commit on November 28th, 2024. As of June 7th, 2025, it has been in development for 6 months:
- Legacy engine (OpenGL): 2 months
- Current engine (Vulkan): 4 months

## Screenshots
The following screenshots document the development of Astrocelerate.

### July 3rd, 2025
![2025-07-03](https://github.com/user-attachments/assets/be22da3f-f431-42d8-ab0a-7d358e144a9e)

### June 21st - 22nd, 2025
![2025-06-22](https://github.com/user-attachments/assets/c27da2fd-ae07-403f-96c3-36c6fb1297b7)
![2025-06-21](https://github.com/user-attachments/assets/0b8aa252-3068-4da2-b433-3886e4e54c60)

### June 8th, 2025
![2025-06-08](https://github.com/user-attachments/assets/645da5cf-9c5b-44c2-aa0b-dfc4be91b824)

### June 3rd, 2025
![2025-06-03](https://github.com/user-attachments/assets/11605157-d576-4e81-a4e1-1356a1696cd4)
![2025-06-03](https://github.com/user-attachments/assets/dd6024d3-3849-4048-b391-93b7ef03b0ae)

### May 20th - 21st, 2025
![2025-05-21](https://github.com/user-attachments/assets/8a92729e-3945-47cd-aa3a-69c8a8d73f0d)
![2025-05-20](https://github.com/user-attachments/assets/2a31b5e5-0b75-41ee-8aea-68cc65536e8d)

### May 16th, 2025
![2025-05-16](https://github.com/user-attachments/assets/bc5a983d-f2a5-40c2-8812-15edb9bc5ac2)

### May 14th, 2025
![2025-05-14](https://github.com/user-attachments/assets/5d99422e-e6bf-4539-84af-b6a8eeedac7f)

### May 4th, 2025
![2025-05-04](https://github.com/user-attachments/assets/5e75e451-e4f2-4f9e-825f-2cb2edf9af55)

### April 1st, 2025
![2025-04-01](https://github.com/user-attachments/assets/4df183fd-ec53-4ec0-a751-0016e6e23de3)

### March 17th, 2025
![2025-03-17](https://github.com/user-attachments/assets/1fca2070-ff43-4595-bd6a-74c8c43ae982)

### December 9th, 2024 (Legacy Astrocelerate)
![2024-12-09](https://github.com/user-attachments/assets/db1f0232-cab3-4022-95f8-75ab503b029c)

