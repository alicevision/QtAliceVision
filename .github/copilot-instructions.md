# Copilot Instructions for QtAliceVision

All comments in this repository must follow the Doxygen conventions. Use Doxygen-style documentation blocks for public APIs, non-trivial logic, classes, methods, signals, properties, and important behavior. Prefer `/** ... */` blocks with tags such as `@brief`, `@param`, `@return`, `@note`, and `@warning` when useful. Avoid undocumented public interfaces and avoid plain, non-Doxygen comments where a Doxygen block is more appropriate.

## Working rules

- Match the project’s existing Qt 6, C++20, and CMake conventions.
- Keep edits focused, readable, and consistent with neighboring code.
- Prefer small, maintainable changes over broad refactors.
- Preserve behavior unless the task explicitly calls for API changes.
- Update documentation when public interfaces or behavior change.
- Never use one liners, always add braces.

## Documentation standards

- Explain intent, constraints, and non-obvious logic with Doxygen rather than vague comments.
- Keep comments factual and concise; avoid restating obvious code.
- Document public methods, classes, and signals near their declaration sites.
- When touching QML-exposed types, ensure the exposed properties and behaviors remain documented.

## Validation

- Run the relevant build or focused validation for the affected area before finishing.
- Keep CMake integration and plugin behavior consistent with the rest of the repository.
- Remove stale or misleading documentation when code changes.
