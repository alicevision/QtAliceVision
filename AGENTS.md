# Agent Guidelines for QtAliceVision

All comments in this repository must follow the Doxygen conventions. Use Doxygen-style documentation blocks for public APIs, non-trivial logic, classes, methods, signals, properties, and important behavior. Prefer `/** ... */` blocks with relevant tags such as `@brief`, `@param`, `@return`, `@note`, and `@warning` when useful. Avoid undocumented public interfaces and avoid plain, non-Doxygen comments where a Doxygen block is more appropriate.

## Project expectations

- Keep changes aligned with the existing Qt 6, C++20, and CMake structure of the project.
- Prefer small, targeted edits that match the style already used in the surrounding code.
- Preserve the current public API behavior unless the task explicitly requires a breaking change.
- Favor readable, idiomatic C++ and QML over clever but opaque solutions.
- Document significant behavior and design intent where it helps future maintainers understand the code.

## Code quality

- Prefer clear names over comments that restate the code.
- Use comments to explain intent, constraints, invariants, and non-obvious rationale.
- Keep documentation factual and concise; do not add redundant boilerplate.
- When modifying a class or method, ensure its Doxygen documentation remains correct and complete.
- For QML-visible items, document exposed properties and behaviors in the corresponding declarations.
- Never use one liners, always add braces.

## Validation

- Validate the relevant build or focused checks for the area being edited before concluding work.
- If a change affects plugin integration, ensure the CMake configuration still builds cleanly and that related code paths remain consistent.
- Do not leave behind stale or misleading comments.

## Scope

These rules apply across the repository, including C++, headers, QML bindings, and plugin-facing code. The goal is to keep the project maintainable, explicit, and easy to navigate for both humans and automation.
