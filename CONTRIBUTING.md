# Contributing to Astrocelerate

This document defines the development standards for Astrocelerate.

---

## Branching

All changes, regardless of size, must be made on a dedicated branch.
Direct pushes to `main` are not permitted.

Branch names follow this format: `type/brief-kebab-case-descriptor`

| Branch Type | Intention |
|---|---|
| `feat/` | New functionality |
| `fix/` | Bug fix |
| `refactor/` | Structural change with no behavior change |
| `docs/` | Documentation only |
| `chore/` | Build system, dependencies, tooling, configuration |
| `perf/` | Performance improvement |
| `test/` | Adding or correcting tests |

Examples:
- `feat/sgp4-propagator`
- `fix/ecs-sparse-set-out-of-bounds`
- `docs/update-installation-guide`

---

## Commits

Astrocelerate follows [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/).

Format: `branch_type(scope): subject`

The [branch type](#branching) specifies what this commit addresses.
The scope identifies which subsystem the commit touches.
The subject is a one-liner summary of changes made in the commit.

When writing a commit message, please keep in mind:
- Type and scope are lowercase
- Subject is imperative mood, no capital first letter, no trailing period
- Commits must be atomic and dedicated to a single scope; if the subject line requires the word "and", it is usually a sign that the commit must be split

### Valid scopes

| Scope | Subsystem |
|---|---|
| `physics` | Physics engine and simulation logic |
| `propagator` | Orbital propagators (SGP4, numerical integrators, etc.) |
| `rendering` | Vulkan rendering layer |
| `ecs` | Entity-component system |
| `ui` | Dear ImGui interface |
| `build` | CMake, build configuration |
| `docs` | Documentation and README |

> [!NOTE]
> The scope is omitted when a change is atomic but genuinely system-wide (e.g., a variable rename that propagates across multiple subsystems), or when it is too generic to be attributed to a single subsystem change (e.g., updating the README).

Examples:
- `feat(propagator): implement SGP4 mean motion initialization`
- `fix(ecs): correct out-of-bounds access in sparse set operator[]`
- `refactor(rendering): extract swapchain logic into dedicated class`
- `chore(build): add Catch2 as test dependency`
- `docs: update installation prerequisites`

---

## Pull Requests

Every change requires a pull request, including documentation edits.

PR titles must follow the same `type(scope): subject` format as commits.
Squash merging is preferred to keep the commit history of the `main` branch linear and readable.

---

## Handling Incidental Discoveries

If a bug, inconsistency, or improvement is noticed outside the current branch's scope during development, it must be logged as a GitHub Issue immediately and left alone until it has its own branch. It does not go into the current branch under any circumstances.

---

## Expanding This Document

If a real commit arises that requires a type or scope not listed here, add it to this document in the same PR that introduces it.
Refrain from adding speculative or hypothetically necessary entries unless absolutely necessary.
