<!-- PROJECT LOGO -->
<br />
<div align="center">
    <a href="https://github.com/ButteredFire/Astrocelerate/">
        <img src="assets/app/ExperimentalAppLogoV1Transparent.png" alt="Logo" width="70%" height="70%">
    </a>
</div>

# About Astrocelerate
## What is it?
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


# Installation
> [!WARNING]
> Astrocelerate has only been tested on Windows, although the application aims to be cross-platform-compatible.
## Generating executable file
- Step 1: Run `SetupRelease.bat`. When the script finishes, the `build` folder will be generated.
- Step 2: Run the executable file in `build/Release`.

## Generating Visual Studio solution file
- Step 1: Open `SetupVisualStudioSolution.bat` in a text editor and edit the Visual Studio major version in the line `cmake -S . -B . -G "Visual Studio 17 2022"` to your own.
- Step 2: Run `SetupVisualStudioSolution.bat`.


# History
Astrocelerate made its first commit on November 28th, 2024. As of May 21st, 2025, it has been in development for 5 months:
- Legacy engine (OpenGL): 2 months
- Current engine (Vulkan): 3 months

## Screenshots
The following screenshots document the development of Astrocelerate.

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

