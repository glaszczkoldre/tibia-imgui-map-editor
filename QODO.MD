# C++ Project Guidelines - Architecture & Style

## 🚫 CRITICAL RULES - NEVER VIOLATE

### 1. ABSOLUTELY NO CODE IN APPLICATION.CPP
- **APPLICATION.CPP IS OFF-LIMITS** for new implementations
- Application.cpp is ONLY for application initialization and main loop coordination
- If you're about to add business logic to Application.cpp → STOP and create a proper module

### 2. SEARCH BEFORE YOU CODE - MANDATORY
Before writing ANY new function, class, or logic block:

```
☐ Search the codebase for similar functionality
☐ Check if this logic already exists (even partially)
☐ Look for utility functions that do related tasks
☐ Check common utility headers (utils/, helpers/, common/)
☐ Verify no duplicate implementations exist
```

**IF SIMILAR CODE EXISTS → REUSE IT, DON'T DUPLICATE IT**

### 3. NO DUPLICATE CODE - ZERO TOLERANCE
- If you find yourself copying logic → create a shared function instead
- If similar patterns exist in 2+ places → refactor to shared utility
- DRY (Don't Repeat Yourself) is NOT optional

---

## 📋 PRE-IMPLEMENTATION CHECKLIST

### Before Adding ANY New Code:

#### Step 1: Research Phase (MANDATORY)
```
☐ What am I trying to implement?
☐ Does this functionality already exist? (grep/search the codebase)
☐ Is there a similar pattern I can follow?
☐ Which module/file SHOULD own this logic?
☐ Have I checked all relevant headers?
```

#### Step 2: Architecture Decision
```
☐ Which existing module does this belong to?
☐ If no module exists, do I need a new one?
☐ Is this a utility function? → goes in utils/ or common/
☐ Is this business logic? → goes in appropriate domain module
☐ Is this UI code? → goes in ui/ or views/
☐ Is this data handling? → goes in models/ or data/
```

#### Step 3: File Placement Rules
```
☐ New class/feature → new .cpp/.h pair in correct module
☐ Utility function → existing or new utility file
☐ Data structure → models/ or types/
☐ Never default to Application.cpp
```

---

## 🏗️ CODE ORGANIZATION & ARCHITECTURE

### Key Principles

#### RAII is Law
- Never manage `GLuint` handles manually
- Every OpenGL object (VAO, VBO, Shader, Texture) must be wrapped in a class that handles `glDelete*` in its destructor

#### Clear Ownership
- Use `std::unique_ptr` for sole ownership
- Use `std::shared_ptr` *only* when ownership is truly shared (e.g., a Texture asset used by multiple Sprites)
- Use raw pointers (`T*`) or references (`T&`) only as non-owning observers

#### Const-Correctness
- Aggressively mark methods `const` if they do not modify internal state

#### Architecture Layering
- Dependencies flow downwards: `UI Layer` (ImGui) → `App/Core` → `Rendering` → `Platform/GL`
- **Never** leak ImGui code into the Core or Rendering layers

#### No Global State
- Avoid singletons or global variables
- Pass dependencies (like Window or Renderer) explicitly via constructor injection

### File Structure Guidelines

#### ✅ CORRECT Module Organization:
```
src/
├── application.cpp          # ONLY app init + main loop
├── modules/
│   ├── feature_name/
│   │   ├── feature_manager.cpp
│   │   ├── feature_manager.h
│   │   └── feature_utils.cpp
├── utils/
│   ├── string_utils.cpp
│   ├── file_utils.cpp
│   └── math_utils.cpp
├── models/
│   └── data_models.cpp
└── ui/
    └── views.cpp
```

#### ❌ WRONG (Never Do This):
```
src/
└── application.cpp          # 5000 lines of everything
```

### Module Ownership Rules

Each module should have:
```
☐ Clear, single responsibility
☐ Its own .cpp/.h pair
☐ Related utilities grouped together
☐ No logic bleeding into Application.cpp
```

---

## 📁 FILE ORGANIZATION

### File Structure
- **One Class Per File:** Unless classes are tightly coupled small helpers
- **Header Extensions:** `.hpp` for headers, `.cpp` for implementation
- **Include Guards:** Use `#pragma once`
- **Forward Declarations:** Use them liberally in headers to reduce compile times and avoid circular dependencies

### Include Order
1. Precompiled Header (if applicable)
2. Corresponding `.hpp` file
3. C++ Standard Library (`<vector>`, `<algorithm>`, `<format>`)
4. Third-party Libs (`<glad/glad.h>`, `<imgui.h>`, `<glm/glm.hpp>`)
5. Project Headers (`"core/Log.hpp"`)

---

## 🔍 CODE REUSE CHECKLIST

### Before Writing a Function:

```
1. ☐ Search codebase: "grep -r 'similar_functionality' src/" or use "desktop commander MCP"
2. ☐ Check existing utility files
3. ☐ Look for similar patterns in related modules
4. ☐ Review recent commits for related work
5. ☐ Ask: "Would this be useful elsewhere?" → Make it reusable
```

### If Similar Code Found:
```
☐ Can I use it directly? → Use it
☐ Can I extend it? → Extend it, don't duplicate
☐ Is it in wrong place? → Refactor ONCE, then use it
☐ Is it slightly different? → Parameterize it to handle both cases
```

---

## 💻 MODERN C++ (C++20/26) GUIDELINES

### Type Deduction & Pointers
- **Use `auto`:** For iterators, complex template types, or when the type is obvious from the RHS (e.g., `auto* t = new Texture();`)
- **Numeric Types:** Be explicit. Use `int32_t`, `uint64_t`, `size_t`, `float`. Avoid generic `int` or `unsigned` in binary file structures
- **Nullptr:** Always use `nullptr`, never `NULL` or `0`

### Line Length
- **Maximum line length:** 120 characters
- C++ templates, namespaces, and verbose OpenGL calls require more horizontal space

### Containers & Views
- **`std::span`:** Use `std::span<T>` instead of passing pointer + size pairs
  - *Example:* `void UploadData(std::span<const uint8_t> data);`
- **`std::string_view`:** Use for read-only string arguments to avoid allocations

### Concepts
- **Use Concepts:** Prefer C++20 `requires` clauses over SFINAE or raw templates for math/grid logic
  ```cpp
  template<typename T>
  requires std::integral<T>
  void SnapToGrid(T& value, T gridSize);
  ```

---

## 🎮 GRAPHICS & UI SPECIFICS

### OpenGL (Glad/GLFW)
- **Math:** Use `glm`. Pass `glm::vec3` by value, `glm::mat4` by `const&`
- **State Machine:** Do not make redundant GL calls. Cache state if necessary, but prefer a "Stateless Renderer" abstraction that sorts draw calls
- **Buffers:** Use `std::vector` to build vertex data, then upload to GPU via `glBufferData`

### Dear ImGui
- **ID Stack:** In loops (like drawing a grid of tiles), **always** use `ImGui::PushID(index)` and `ImGui::PopID()`
- **Strings:** Use `const char*` literals where possible. Use `std::format` (C++20) for dynamic labels
  - *Good:* `ImGui::Text("Pos: %d, %d", x, y);` (ImGui internal formatting)
  - *Good:* `ImGui::TextUnformatted(std::format("Count: {}", count).c_str());`

---

## 📝 DOCUMENTATION & COMMENTS

- **Doxygen Style:** Use `/*` for comment blocks and `/*` for API documentation.
- **Public Interface:** Document all public methods in the `.hpp` file
- **Implementation:** Comment complex algorithms (e.g., auto-tiling logic) inside the `.cpp`
- **TODOs:** format as `// TODO(User): Description`

---

## 🎯 IMPLEMENTATION WORKFLOW

### The Correct Process:

```
1. UNDERSTAND the requirement
   ↓
2. SEARCH for existing implementations
   ↓
3. DECIDE on correct module/file location
   ↓
4. CHECK if new file is needed
   ↓
5. IMPLEMENT in the RIGHT place
   ↓
6. VERIFY no duplication created
   ↓
7. UPDATE relevant headers
```

### Questions to Ask BEFORE Coding:

```
☐ "Where does this logically belong?"
☐ "Does anything similar exist?"
☐ "Am I creating a new responsibility that needs a new module?"
☐ "Will this be used by multiple parts of the codebase?"
☐ "Am I following the existing architecture patterns?"
```

---

## 🚨 RED FLAGS - STOP IF YOU SEE THESE

### Immediate Stop Signals:

```
❌ "I'll just add this to Application.cpp"
❌ "This is similar to X, but I'll write it from scratch"
❌ "I'll copy this function and modify it slightly"
❌ "I'll put everything in one file for now"
❌ "I don't know where this goes, so Application.cpp"
```

---

## 🔄 REFACTORING TRIGGER POINTS

Refactor immediately when:

```
☐ Same logic appears in 2+ places
☐ Function exceeds 50 lines
☐ File exceeds 500 lines
☐ Application.cpp gains ANY business logic
☐ You copy-paste ANY code
```

---

## ✅ VALIDATION CHECKLIST

Before submitting ANY code:

```
☐ No duplicate code exists
☐ Nothing added to Application.cpp except wiring
☐ Code is in logically correct module
☐ Existing utilities were checked and reused
☐ New code follows existing patterns
☐ Headers are properly organized
☐ No monolithic functions (>50 lines)
☐ RAII principles followed for all OpenGL objects
☐ Proper ownership semantics (unique_ptr/shared_ptr/raw pointers)
☐ Const-correctness maintained
☐ No global state introduced
```

---

## 🎓 REMEMBER

1. **Application.cpp is NOT a dumping ground**
2. **Search BEFORE you code**
3. **Reuse ALWAYS beats rewrite**
4. **Organization prevents technical debt**
5. **RAII is law for OpenGL objects**
6. **Clear ownership prevents memory leaks**
7. **Const-correctness catches bugs early**
8. **Your future self will thank you**

---

## 🔧 QUICK REFERENCE

**Before writing ANY code, ask:**
- Does this already exist?
- Where should this live?
- Am I about to duplicate something?
- Is Application.cpp the right place? (Answer: NO)
- Does this OpenGL object have RAII wrapper?
- Is ownership clear (unique_ptr/shared_ptr/raw pointer)?

**The mantra:** SEARCH → REUSE → ORGANIZE → IMPLEMENT
