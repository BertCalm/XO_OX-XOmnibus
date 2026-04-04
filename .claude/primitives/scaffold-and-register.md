# Primitive: Scaffold and Register

Create new files from a template or pattern, then wire them into the project's registry/index/manifest.

## Pattern

```
scaffold(template, variables)
  → create files from template with variables substituted
register(target, registry)
  → add the new component to the project's index/factory/manifest
verify(target)
  → confirm the new component is discoverable by the system
```

## Inputs

- **template**: The file pattern or archetype to copy from
- **variables**: Name, ID, prefix, color, or other substitution values
- **registry**: The index file, factory, manifest, or import map to update
- **verification**: How to confirm registration worked (build, test, list command)

## Outputs

- New files created at correct paths
- Registry updated with new entry
- Verification result (pass/fail)

## Usage Examples

**XOceanus engine:**
```
template: Source/Engines/{Name}/{Name}Engine.h + .cpp
variables: {Name: "Origami", prefix: "origami_", color: "#E63946"}
registry: Source/Core/EngineRegistry.h (REGISTER_ENGINE macro)
verify: cmake --build build (compiles without errors)
```

**Generic (any project):**
```
template: src/components/{Name}/{Name}.tsx + {Name}.test.tsx
variables: {Name: "UserProfile"}
registry: src/components/index.ts (export statement)
verify: npm run build
```

## Portability Notes

Nearly every project has a "create a new module and wire it in" workflow. The specifics vary (React component, API route, database migration, engine adapter) but the shape is always: **template → create → register → verify**.
