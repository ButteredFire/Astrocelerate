> [!NOTE]
> Đây là bản README tiếng Anh. Vui lòng đọc bản tiếng Việt tại [đây](README-VN.md).

<div align="center" class="grid cards" style="display: flex; align-items: center; justify-content: center; text-decoration: none;" markdown>
    <img src="assets/App/AstrocelerateLogo.png" alt="Logo" width="75%">
    <p>Logo © 2024-2025 <a href="https://www.linkedin.com/in/minhduong-thechosenone/">Minh Duong</a>, D.B.A. <b>Oriviet Aerospace</b></p>
    </br>
    <p>Join our communities!</p>
    <a href="https://www.reddit.com/r/astrocelerate" target="_blank"><img alt="Reddit" src="https://img.shields.io/badge/Reddit-grey?style=flat&logo=reddit&logoColor=%23FF4500&logoSize=auto"></a>
    <a href="https://discord.gg/zZNNhW65DC" target="_blank"><img alt="Discord" src="https://img.shields.io/badge/Discord-grey?style=flat&logo=discord&logoColor=%235865F2&logoSize=auto"></a>
</div>



# Table of Contents
* [Goal & Vision](#goal--vision)
* [Features](#features)
    * [Currently implemented](#currently-implemented)
    * [Near-future plans](#near-future-plans)
* [Installation](#installation)
    * [Prerequisites](#prerequisites)
    * [Bootstrapping](#bootstrapping)
* [History](#history)
    * [Screenshots](#screenshots)

---

Astrocelerate is a high-performance orbital mechanics and spaceflight simulation engine, designed as a versatile and intuitive tool that bridges the gap between "arcane" professional aerospace software and the accessibility of modern engines.

# Goal & Vision
Most aerospace software is either simplified and gamified (e.g., Kerbal Space Program) or a legacy tool built on decades-old Fortran/C codebases (e.g., GMAT/STK). Astrocelerate aims to be:

- Scientifically Valid: Utilizing NASA's SPICE kernels for high-fidelity ephemeris and coordinate systems;
- Approachable: Enabling hobbyists and non-programmers to design complex orbital maneuvers with a friendly modern UI and Visual Scripting system;
- Versatile: Boasting a simulation suite sufficient for most simulation needs;
- Performant: Built for massive-scale simulations (constellations, asteroid belts) using multi-threaded C++ and GPU acceleration;
- Open-Source: Contributing to the open-source ecosystem and overall growth in the aerospace community with publicly available source code, documentation, and support for plugins.

Astrocelerate is developed with the belief that humanity's greatest frontier should be open, modern, and independent of proprietary, high-cost ecosystems. It is a tool for students, researchers, and engineers to build the future of orbital infrastructure.

*Astrocelerate is an initiative and first project of [Oriviet Aerospace](https://www.oriviet.org), an early-stage startup based in Vietnam aiming to build the next generation of spaceflight simulation tools for engineers, researchers, and space mission designers.*

# Features
## Currently implemented
- Real-time orbital propagation
- Accurate 2-body simulation
- Configurable physics time step
- Reference frame system with fixed simulation scaling (maintains physical accuracy across polarized scales)
- Custom ECS-based architecture with sparse-set storage (allows for performant simulation and efficient data flow between the GUI and backend)
- Live telemetry data, console, etc.; Tweakable simulation settings
- Offscreen rendering (allows for custom viewports, post-processing, etc.)
- N-body simulation capabilities
- A more advanced model loader, with better, correctly mapped textures and visual fidelity
- Solvers, coordinate systems, epochs

## Near-future plans
- Swappable numerical integrators (Symplectic Euler, RK4)
- Data-caching in ECS for even more performance
- Compute shaders and the offloading of parallelizable processes to the GPU
- Dynamic scaling for seamless transitions (e.g., from terrain to planet)
- A more diverse range of numerical integration methods (Verlet, Simpson's Rule, Gaussian quadrature)
- Visual scripting to allow for user-created simulations
- Serialization of GUI and simulation data, with basic exporting capabilities


# Installation
> [!WARNING]
> Astrocelerate has only been tested on Windows, although the application aims to be cross-platform-compatible.

## Prerequisites
- Vulkan SDK (Vulkan 1.2+)
- CSPICE Toolkit N0067
- Vcpkg dependency manager
- CMake 3.30+
- Python 3.9+
- C++20

## Bootstrapping
- Open `CMakePresets.json` and configure environment variables for both Debug and Release builds (like `SPICE_ROOT_DIR`) to their appropriate paths.
- Run `SetupDebug.*` to set up a Debug configuration, or `SetupRelease.*` to set up a Release configuration, according to your operating system.
- Alternatively, you can manually run `GenerateDirectories.bat` to ensure file discovery is up-to-date, then run `scripts/Bootstrap.py` and follow on-screen instructions.


# History
Astrocelerate made its first commit on November 28, 2024. As of December 28, 2025, it has been in development for 395 days. During its first two months in existence, Astrocelerate was written in OpenGL, but it henceforth migrated to Vulkan due to inherent limitations with OpenGL's state-machine programming paradigm.

## Screenshots
The following screenshots document the development of Astrocelerate (Date format: DD/MM/YYYY).

### 17/12/2025
<img width="1919" height="1031" alt="2025-12-17" src="https://github.com/user-attachments/assets/82a93f52-c7ea-45d6-a5eb-f03534892a36" />

### 20/10/2025
<img width="1919" height="1029" alt="2025-10-20" src="https://github.com/user-attachments/assets/6a0309a0-6af9-4c2a-a9a3-4774f811d2b6" />

### 18/10/2025 (after 2-month hiatus)
<img width="1919" height="1030" alt="2025-10-18" src="https://github.com/user-attachments/assets/723a596f-fc3f-4507-9b71-cc26dbe89690" />

### 16/08/2025
<img width="1919" height="1031" alt="2025-08-16" src="https://github.com/user-attachments/assets/d0979618-2afa-4734-81da-76b5b4051492" />

### 25/07/2025
<img width="1919" height="1031" alt="2025-07-25" src="https://github.com/user-attachments/assets/fb586a14-5ddc-4442-a18a-12a35fca709e" />
<img width="1919" height="1032" alt="2025-07-25" src="https://github.com/user-attachments/assets/958aa972-6ba2-4740-b331-321d3da697ed" />

### 10/07/2025
![2025-07-10](https://github.com/user-attachments/assets/fb0ea6af-8cf1-43de-9279-71a61d7c1744)

### 03/07/2025
![2025-07-03](https://github.com/user-attachments/assets/be22da3f-f431-42d8-ab0a-7d358e144a9e)

### 21/06 - 22/06/2025
![2025-06-22](https://github.com/user-attachments/assets/c27da2fd-ae07-403f-96c3-36c6fb1297b7)
![2025-06-21](https://github.com/user-attachments/assets/0b8aa252-3068-4da2-b433-3886e4e54c60)

### 08/06/2025
![2025-06-08](https://github.com/user-attachments/assets/645da5cf-9c5b-44c2-aa0b-dfc4be91b824)

### 03/06/2025
![2025-06-03](https://github.com/user-attachments/assets/11605157-d576-4e81-a4e1-1356a1696cd4)
![2025-06-03](https://github.com/user-attachments/assets/dd6024d3-3849-4048-b391-93b7ef03b0ae)

### 20/05 - 21/05/2025
![2025-05-21](https://github.com/user-attachments/assets/8a92729e-3945-47cd-aa3a-69c8a8d73f0d)
![2025-05-20](https://github.com/user-attachments/assets/2a31b5e5-0b75-41ee-8aea-68cc65536e8d)

### 16/05/2025
![2025-05-16](https://github.com/user-attachments/assets/bc5a983d-f2a5-40c2-8812-15edb9bc5ac2)

### 14/05/2025
![2025-05-14](https://github.com/user-attachments/assets/5d99422e-e6bf-4539-84af-b6a8eeedac7f)

### 04/05/2025
![2025-05-04](https://github.com/user-attachments/assets/5e75e451-e4f2-4f9e-825f-2cb2edf9af55)

### 01/04/2025
![2025-04-01](https://github.com/user-attachments/assets/4df183fd-ec53-4ec0-a751-0016e6e23de3)

### 17/03/2025
![2025-03-17](https://github.com/user-attachments/assets/1fca2070-ff43-4595-bd6a-74c8c43ae982)

### 09/12/2024 (Legacy Astrocelerate)
![2024-12-09](https://github.com/user-attachments/assets/db1f0232-cab3-4022-95f8-75ab503b029c)
