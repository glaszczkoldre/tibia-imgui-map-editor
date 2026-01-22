

<h1 align="center">TIME - Tibia ImGui Map Editor</h1>



<p align="center">
  <strong>A Modern, Community-Driven 2D Tile-Based Map Editor for Open Tibia.</strong>
</p>

<img width="2554" height="1387" alt="obraz" src="https://github.com/user-attachments/assets/dac2c242-a2f3-4a9f-b2a3-23929fd90c3b" />



<p align="center">
  <a href="#"><img src="https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus" alt="C++20"></a>
  <a href="#"><img src="https://img.shields.io/badge/OpenGL-3.3%2B-5586A4?logo=opengl" alt="OpenGL"></a>
</p>

<p align="center">
  <a href="#quick-start">Quick Start</a> •
  <a href="#features">Features</a> •
  <a href="#architecture">Architecture</a> •
  <a href="#building">Building</a> •
  <a href="#contributing">Contributing</a>
</p>

<p align="center">
  <strong>Disclaimer:</strong>
</p>
<p align="center">
  <strong>This application was developed entirely by various Agentic AI tools.</strong> </br>
  <strong>Every vibe-coder is welcome to contribute, as project rules and architecture are followed..</strong>
</p>


---


## Project Overview

### Vision & Mission

**ImGui Map Editor** is community-driven map editor for Open Tibia, designed from the ground up to deliver exceptional performance, a modern user experience, and seamless cross-platform compatibility. My mission is to empower map creators, server administrators, and content developers with a professional-grade tool that makes map creation intuitive, efficient, and enjoyable.

Built upon the foundation of **Remere's Map Editor (RME)**, this project represents a fundamental reimagining of what a Open Tibia map editor can be. With modern C++20 standards, hardware-accelerated OpenGL rendering, and the battle-tested ImGui framework to create an editor that feels responsive and polished.

**Core Principles:**
- **Performance First**: Chunk-based spatial storage, sprite batching, and multi-draw indirect rendering ensure buttery-smooth editing even on massive maps
- **Modern Architecture**: Clean separation of concerns with service-oriented design, RAII resource management, and dependency injection
- **Community-Driven**: Open development process, welcoming contributions from the Tibia mapping community.
- **Cross-Platform**: Native support for Windows, Linux, and macOS

### What Makes This Editor Unique

| Feature | RME Classic | ImGui Map Editor |
|---------|-------------|------------------|
| **Rendering** | wxWidgets/GDI | Modern OpenGL 3.3+ |
| **UI Framework** | wxWidgets | Dear ImGui (Docking) |
| **Sprite Loading** | Synchronous | Async Background Loading |
| **Map Storage** | Flat arrays | Chunked Spatial Index |
| **Multi-Floor View** | Basic | Ghost Floor Visualization |
| **Light Simulation** | Limited | Real-time Dynamic Lighting |
| **Undo System** | Simple | Compressed Tile Snapshots |
| **Architecture** | Monolithic | Service-Oriented Modules |

**Key Differentiators:**
- **Hardware-Accelerated Rendering**: OpenGL-based pipeline with sprite atlasing and batch rendering achieves 60+ FPS on large maps
- **Async Sprite Loading**: Background thread loads sprites on-demand, keeping the UI responsive
- **Ghost Floor Preview**: See adjacent floors with configurable transparency for multi-level editing
- **Real-Time Lighting**: Dynamic light overlay simulates in-game lighting conditions
- **Ribbon Interface**: Clean, organized toolbar with logical grouping of features

### Target Audience

- **Map Creators**: Build immersive game worlds with professional tools
- **Server Administrators**: Maintain and extend existing maps efficiently
- **Content Developers**: Create custom tilesets, brushes, and palettes
- **Open Tibia Contributors**: Help push the boundaries of map editing technology

### Development Status

> [!IMPORTANT]
> **Current Version is (~50% Complete)**

This project is actively developed and released for **collaborative community development**. While many core features are functional, not all RME features have been ported yet By the time beeing - this is NOT RME replacement, until first RC release.

### Heritage & Acknowledgments

 **Remere's Map Editor**, **Item Editor** and **Object Builder** has been the gold standard for Tibia map editing for over a decade, and our architecture draws heavy inspiration from its proven design patterns.

We acknowledge and thank:
- The original developers of open tibia tools (**Remere's Map Editor, Object Builder, Item Editor, Otclient/V8** )
- The **Open Tibia** community for file format specifications
- All contributors who have helped shape this project

---

## Quick Start

```bash
# Clone the repository
git clone https://github.com/karolak6612/ImguiMapEditor.git
cd ImguiMapEditor/ImguiMapEditor

# Build (Windows with vcpkg)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Run
./build/Release/TibiaMapEditor.exe
```

**First Launch:**
1. Select your Tibia client version (7.1 through 10.10+)
2. Point to your client data directory (containing `.dat` and `.spr` files)
3. Create a new map or open an existing `.otbm` file


---

## Architecture Overview

### High-Level Architecture

```
┌────────────────────────────────────────────────────────────────────┐
│                         PRESENTATION LAYER                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │   MainWindow │  │   MenuBar    │  │   Dialogs    │              │
│  └──────────────┘  └──────────────┘  └──────────────┘              │
├────────────────────────────────────────────────────────────────────┤
│                           UI LAYER (ImGui)                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐            │
│  │  Ribbon  │  │  Panels  │  │ Dialogs  │  │ Widgets  │            │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘            │
├────────────────────────────────────────────────────────────────────┤
│                       CONTROLLER LAYER                             │
│  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐        │
│  │ MapInputCtrl   │  │  HotkeyCtrl    │  │  SearchCtrl    │        │
│  └────────────────┘  └────────────────┘  └────────────────┘        │
├────────────────────────────────────────────────────────────────────┤
│                    APPLICATION / CORE LAYER                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │ Application  │  │ EditorSession│  │ BrushSystem  │              │
│  └──────────────┘  └──────────────┘  └──────────────┘              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │ TabManager   │  │ StateManager │  │ Lifecycle    │              │
│  └──────────────┘  └──────────────┘  └──────────────┘              │
├────────────────────────────────────────────────────────────────────┤
│                        SERVICES LAYER                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌───────────┐  │
│  │ ClientData  │  │SpriteManager│  │ Clipboard   │  │  Preview  │  │
│  └─────────────┘  └─────────────┘  └─────────────┘  └───────────┘  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌───────────┐  │
│  │ BrushLookup │  │  Selection  │  │  MapOps     │  │  Settings │  │
│  └─────────────┘  └─────────────┘  └─────────────┘  └───────────┘  │
├────────────────────────────────────────────────────────────────────┤
│                         DOMAIN LAYER                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │  ChunkedMap  │  │    Tile      │  │    Item      │              │
│  └──────────────┘  └──────────────┘  └──────────────┘              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │   History    │  │  Selection   │  │   CopyBuf    │              │
│  └──────────────┘  └──────────────┘  └──────────────┘              │
├────────────────────────────────────────────────────────────────────┤
│                        RENDERING LAYER                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │ RenderMgr    │  │   Passes     │  │  Overlays    │              │
│  └──────────────┘  └──────────────┘  └──────────────┘              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │    Light     │  │   Camera     │  │   Minimap    │              │
│  └──────────────┘  └──────────────┘  └──────────────┘              │
├────────────────────────────────────────────────────────────────────┤
│                           I/O LAYER                                │
│  ┌───────────┐  ┌───────────┐  ┌───────────┐  ┌───────────┐        │
│  │   OTBM    │  │    OTB    │  │  DAT/SPR  │  │    XML    │        │
│  └───────────┘  └───────────┘  └───────────┘  └───────────┘        │
├────────────────────────────────────────────────────────────────────┤
│                        PLATFORM LAYER                              │
│  ┌──────────────────────────────────────────────────────────┐      │
│  │             GLFW + OpenGL + Native File Dialogs          │      │
│  └──────────────────────────────────────────────────────────┘      │
└────────────────────────────────────────────────────────────────────┘
```

**Dependency Flow:** UI Layer → Controllers → Core layer → Services → Domain → I/O → Platform

> [!WARNING]
> Dependencies flow **downwards only**. 

### Core Application Layer

The application core resides in `Application/` and manages the editor lifecycle:

| Component | Responsibility |
|-----------|----------------|
| `Application` | Main orchestrator, initialization, main loop |
| `AppStateManager` | FSM for Startup ↔ Editor transitions |
| `EditorSession` | Per-map editing session with undo/services |
| `MapTabManager` | Multi-map tab management |
| `SessionLifecycleManager` | Session creation, destruction, version switching |
| `ClientVersionManager` | Client version state and validation |
| `CallbackMediator` | Decoupled event/callback routing |
| `MapOperationHandler` | High-level map operations (save, load, export) |

**Application Lifecycle:**

```
┌─────────────┐    ┌──────────────┐    ┌─────────────┐
│   Startup   │ -> │ Version      │ -> │   Editor    │
│   Dialog    │    │ Selection    │    │   Mode      │
└─────────────┘    └──────────────┘    └─────────────┘
                          │
                          v
                   ┌──────────────┐
                   │ Load Assets  │
                   │ (DAT/SPR/OTB)│
                   └──────────────┘
```

### Domain Layer

The domain layer (`Domain/`) contains pure business logic with no external dependencies:

**ChunkedMap System:**

```cpp
// Hierarchical spatial storage for O(1) tile access
ChunkedMap
  └── FloorMap[0..15]          // 16 floors
        └── Chunk[x,y]          // 32x32 tile chunks
              └── Tile[0..1023] // Contiguous tile storage
```

**Key Domain Classes:**

| Class | Purpose |
|-------|---------|
| `ChunkedMap` | Root container with chunk-based spatial indexing |
| `Tile` | Single map tile with ground, items, creatures, spawn |
| `Item` | Item instance with attributes (action ID, unique ID, etc.) |
| `Creature` | Creature instance with outfit data |
| `Position` | Immutable (x, y, z) coordinate tuple |
| `House` | House definition with tiles and properties |
| `Spawn` | Spawn point with radius and creature list |
| `Town` | Town definition with temple position |

**History System:**

```
HistoryManager
  └── HistoryBuffer (circular, memory-capped)
        └── HistoryEntry[]
              └── TileSnapshot (compressed delta)
```

The undo system uses **tile snapshots** with LZ4 compression to minimize memory usage while supporting unlimited undo depth (configurable cap).

### Brush System

The brush system (`Brushes/`) is a proof of concept. Its not functional. 



### Rendering Pipeline

The rendering system (`Rendering/`) uses modern OpenGL with a multi-pass architecture:

```
RenderOrchestrator
  └── RenderingManager
        └── TileRenderer    ─────────────────────────┐
              │                                       │
              ├── TerrainPass        (Base layer)     │
              ├── SpawnTintPass      (Spawn overlay)  │
              ├── GhostFloorRenderer (Adjacent floors)│
              ├── LightingPass       (Dynamic lights) │
              └── OverlayRenderer    ─────────────────┤
                    ├── GridOverlay                   │
                    ├── SelectionOverlay              │
                    ├── PreviewOverlay                │
                    ├── TooltipOverlay                │
                    ├── SpawnLabelOverlay             │
                    ├── WaypointOverlay               │
                    └── StatusOverlay                 │
                                                      │
              SpriteBatcher       ◄───────────────────┘
                └── RingBuffer (GPU staging)
                      └── SpriteAtlas (Texture packing)
```

**Rendering Optimizations:**

| Technique | Benefit |
|-----------|---------|
| Chunk-level culling | Skip invisible chunks entirely |
| Sprite batching | Minimize draw calls |
| Ring buffer | Lock-free GPU uploads |
| Atlas packing | Reduce texture binds |
| Async sprite loading | Non-blocking asset loading |

**Light System:**

```
LightManager
  ├── LightGatherer       // Collect light sources in view
  ├── LightCache          // Memoize light calculations
  ├── LightTexture        // GPU light lookup texture
  └── LightOverlay        // Final compositing
```

### I/O Systems

The I/O layer (`IO/`) handles all file format parsing and serialization:

**Binary Formats:**

| Reader/Writer | Format | Purpose |
|---------------|--------|---------|
| `OtbmReader/Writer` | `.otbm` | Open Tibia Binary Map |
| `OtbReader` | `.otb` | Item type definitions |
| `SprReader` | `.spr` | Sprite graphics |
| `DatReaderV*` | `.dat` | Item/creature metadata |
| `SecReader` | `.sec` | Secondary client data |
| `NodeFileReader` | Binary nodes | Hierarchical binary format |

**Dat Reader Hierarchy:**

```
DatReaderBase (Abstract)
  ├── DatReaderV710    (Tibia 7.10)
  ├── DatReaderV740    (Tibia 7.40)
  ├── DatReaderV755    (Tibia 7.55)
  ├── DatReaderV780    (Tibia 7.80)
  ├── DatReaderV860    (Tibia 8.60)
  └── DatReaderV1010   (Tibia 10.10+)
```

**XML Formats:**

| Reader/Writer | Format | Purpose |
|---------------|--------|---------|
| `BrushXmlReader` | `brushes.xml` | Brush definitions |
| `CreatureXmlReader` | `creatures.xml` | Creature types |
| `ItemXmlReader` | `items.xml` | Item metadata |
| `TilesetXmlReader/Writer` | `tilesets.xml` | Tileset organization |
| `PaletteXmlReader` | `palettes.xml` | Palette definitions |
| `HouseXmlReader/Writer` | `houses.xml` | House data |
| `SpawnXmlReader/Writer` | `spawns.xml` | Spawn data |
| `MaterialsXmlReader` | `materials.xml` | Material definitions |

### Services Layer

The services layer (`Services/`) provides business logic and utilities:

**Core Services:**

| Service | Responsibility |
|---------|----------------|
| `ClientDataService` | Item types, object attributes |
| `SpriteManager` | Sprite loading, caching, GPU upload |
| `SpriteAsyncLoader` | Background sprite loading queue |
| `ClipboardService` | Copy/paste operations |
| `BrushSettingsService` | Brush size, shape, parameters |
| `ConfigService` | Application configuration |
| `HotkeyRegistry` | Keyboard shortcut management |
| `ViewSettings` | Rendering preferences |
| `TilesetService` | Tileset and palette management |
| `CreatureSpriteService` | Creature outfit rendering |
| `CreatureSimulator` | Creature animation simulation |

**Map Services:**

| Service | Responsibility |
|---------|----------------|
| `MapLoadingService` | Map file loading orchestration |
| `MapSavingService` | Map serialization |
| `MapEditingService` | Tile modification operations |
| `MapSearchService` | Search and replace |
| `MapCleanupService` | Remove invalid items, fix errors |
| `MapMergeService` | Combine multiple maps |

**Brush Lookup Services:**

| Service | Purpose |
|---------|---------|
| `BorderLookupService` | Border/transition tile lookup |
| `WallLookupService` | Wall segment lookup |
| `CarpetLookupService` | Carpet pattern lookup |
| `TableLookupService` | Table arrangement lookup |

**Preview System:**

```
PreviewService
  ├── PreviewManager           // Coordinates preview providers
  └── Providers/
        ├── ItemPreviewProvider
        ├── CreaturePreviewProvider
        ├── DoodadPreviewProvider
        ├── GroundPreviewProvider
        └── WallPreviewProvider
```

### UI Framework

The UI layer (`UI/`) uses Dear ImGui with a docking-enabled layout:

**Component Hierarchy:**

```
UI/
├── Core/
│     └── Theme system
├── Ribbon/
│     ├── RibbonController
│     └── Panels/
│           ├── FilePanel
│           ├── EditPanel
│           ├── ViewPanel
│           ├── BrushesPanel
│           ├── PalettesPanel
│           └── SelectionPanel
├── Panels/
│     └── MapPanel (main editing viewport)
├── Widgets/
│     ├── TilesetWidget
│     ├── TilesetGridWidget
│     └── SpritePicker
├── Dialogs/
│     ├── Startup/
│     │     ├── StartupDialog
│     │     └── VersionSelectionDialog
│     ├── Properties/
│     │     ├── TilePropertiesDialog
│     │     └── ItemPropertiesDialog
│     ├── Import/
│     ├── ClientConfiguration/
│     ├── NewMapDialog
│     ├── EditTownsDialog
│     ├── AdvancedSearchDialog
│     └── ConfirmationDialog
└── Windows/
      ├── MinimapWindow
      ├── BrowseTileWindow
      ├── IngameBoxWindow
      └── PaletteWindowManager
```

### Controllers

Controllers (`Controllers/`) handle user input and coordinate UI with services:

| Controller | Responsibility |
|------------|----------------|
| `MapInputController` | Mouse/keyboard input on map viewport |
| `HotkeyController` | Global keyboard shortcut handling |
| `SearchController` | Search dialog coordination |
| `SimulationController` | Creature animation playback |
| `StartupController` | Startup flow orchestration |
| `WindowController` | Window state management |
| `WorkspaceController` | Panel layout management |

---

## Technology Stack

### Core Technologies

| Technology | Version | Purpose |
|------------|---------|---------|
| **C++** | C++20 | Core language |
| **CMake** | 3.20+ | Build system |
| **vcpkg / Conan** | Latest | Package management |

### Graphics Stack

| Library | Purpose |
|---------|---------|
| **OpenGL** | 3.3+ Core Profile rendering |
| **GLFW** | Window creation, input handling |
| **GLAD** | OpenGL function loading |
| **GLM** | Mathematics (vectors, matrices) |
| **stb_image** | Image loading |

### UI Framework

| Library | Purpose |
|---------|---------|
| **Dear ImGui** | Immediate-mode GUI (docking branch) |
| **FontAwesome 6** | Icon font |
| **ImGuiNotify** | Toast notifications |
| **imHotKey** | Hotkey binding UI |
| **nativefiledialog-extended** | Native file dialogs |

### Utility Libraries

| Library | Purpose |
|---------|---------|
| **spdlog** | Logging |
| **nlohmann_json** | JSON parsing |
| **pugixml** | XML parsing |
| **fmt** | String formatting |
| **LZ4** | Fast compression |
| **Boost.IOStreams** | Stream utilities |
| **Boost.Filesystem** | Path handling |
| **Boost.CircularBuffer** | History buffer |
| **zlib** | Compression |

---

## Building & Setup

### Prerequisites

- **C++ Compiler**: MSVC 2022, GCC 11+, or Clang 14+
- **CMake**: 3.20 or newer
- **Git**: For cloning and submodule management
- **GPU**: OpenGL 3.3+ capable graphics card

### Build with vcpkg (Default)

vcpkg is automatically integrated via CMake manifest mode.

**Windows (Visual Studio):**

```powershell
# Clone the repository
git clone https://github.com/karolak6612/ImguiMapEditor.git
cd ImguiMapEditor/ImguiMapEditor

# Configure (vcpkg will auto-install dependencies)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# The executable is at: build/Release/TibiaMapEditor.exe
```

**Linux/macOS:**

```bash
# Ensure vcpkg is installed and VCPKG_ROOT is set
export VCPKG_ROOT=/path/to/vcpkg

cmake -S . -B build \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build

# The executable is at: build/TibiaMapEditor
```

### Build with Conan

```bash
# Install Conan
pip install conan
conan profile detect  # First time only

cd ImguiMapEditor

# Install dependencies
conan install . --output-folder=build --build=missing -s build_type=Release

# Configure
cmake -S . -B build \
    -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release
```

### Troubleshooting Build Issues

| Issue | Solution |
|-------|----------|
| vcpkg not found | Set `VCPKG_ROOT` environment variable |
| OpenGL headers missing | Ensure GPU drivers are installed |
| Compiler too old | Upgrade to MSVC 2022, GCC 11+, or Clang 14+ |
| CMake version | Upgrade to CMake 3.20+ |
| Boost linking errors | Clean build directory and rebuild vcpkg |

### Client Data Setup

The editor requires Tibia client data files to function:

1. **Obtain Client Files**: You need `.dat`, `.spr`, and `.otb` files from a Tibia client
2. **Supported Versions**: 7.10 through 10.10+ (see `clients.json` for full list)
3. **Directory Structure**:

```
client_data/
├── 854/                    # Version-specific folder
│   ├── Tibia.dat
│   ├── Tibia.spr
│   └── items.otb
├── 1010/
│   ├── Tibia.dat
│   ├── Tibia.spr
│   └── items.otb
└── ...
```

4. **Configuration**: On first launch, use the Client Configuration dialog to point to your client data directories

---

## Usage Guide

### Getting Started

**First Launch:**

1. The **Startup Dialog** appears on launch
2. Click **"Configure Clients"** to set up your client data paths
3. Select a **client version** for your new map
4. Choose **"New Map"** or **"Open Map"**

**Interface Overview:**

```
┌─────────────────────────────────────────────────────────────────┐
│  Ribbon Toolbar (File | Edit | View | Brushes | Palettes | ...)  │
├─────────────┬───────────────────────────────────┬───────────────┤
│             │                                   │               │
│   Palette   │        Map Viewport               │   Minimap     │
│   Panel     │                                   │               │
│             │                                   ├───────────────┤
│             │                                   │               │
│             │                                   │  Tile         │
│             │                                   │  Inspector    │
│             │                                   │               │
└─────────────┴───────────────────────────────────┴───────────────┘
│                        Status Bar                                │
└──────────────────────────────────────────────────────────────────┘
```

#
### Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| New Map | `Ctrl+N` |
| Open Map | `Ctrl+O` |
| Save Map | `Ctrl+S` |
| Save As | `Ctrl+Shift+S` |
| Undo | `Ctrl+Z` |
| Redo | `Ctrl+Y` |
| Copy | `Ctrl+C` |
| Paste | `Ctrl+V` |
| Cut | `Ctrl+X` |
| Delete | `Delete` |
| Select All | `Ctrl+A` |
| Floor Up | `Page Up` |
| Floor Down | `Page Down` |
| Zoom In | `Mouse Wheel Up` |
| Zoom Out | `Mouse Wheel Down` |
| Toggle Grid | `G` |
| Toggle Minimap | `M` |
| Search | `Ctrl+F` |
| Preferences | `Ctrl+,` |

*Hotkeys are customizable via Edit → Preferences → Hotkeys*

---

## Code Structure Guide

### Directory Organization

```
ImguiMapEditor/
├── Application/           # Core application lifecycle
│   ├── Coordination/      # Version switching, session coordination
│   └── Selection/         # Selection state management
├── Brushes/               # Painting tools
│   ├── Behaviors/         # Brush behavior strategies
│   ├── Core/              # Base brush interfaces
│   ├── Data/              # Brush data structures
│   ├── Enums/             # Brush enumerations
│   └── Types/             # Concrete brush implementations
├── Controllers/           # Input handling and UI coordination
├── Core/                  # Configuration and constants
├── Domain/                # Business logic and data models
│   ├── Algorithms/        # Map algorithms
│   ├── History/           # Undo/redo system
│   ├── Palette/           # Palette definitions
│   ├── Search/            # Search data structures
│   ├── Selection/         # Selection data structures
│   └── Tileset/           # Tileset definitions
├── IO/                    # File format handlers
│   ├── Flags/             # Item flags
│   ├── Otbm/              # OTBM format
│   ├── Readers/           # DAT version readers
│   └── Sec/               # SEC format
├── Input/                 # Low-level input handling
├── Platform/              # OS abstraction
├── Presentation/          # High-level UI shells
│   └── Dialogs/           # Modal dialogs
├── Rendering/             # Graphics pipeline
│   ├── Animation/         # Sprite animation
│   ├── Backend/           # OpenGL wrappers
│   ├── Camera/            # View camera
│   ├── Core/              # Renderer core
│   ├── Frame/             # Frame orchestration
│   ├── Light/             # Lighting system
│   ├── Map/               # Map rendering
│   ├── Minimap/           # Minimap rendering
│   ├── Overlays/          # UI overlays
│   ├── Passes/            # Render passes
│   ├── Resources/         # GPU resources
│   ├── Selection/         # Selection rendering
│   ├── Tile/              # Tile rendering
│   ├── Utils/             # Rendering utilities
│   └── Visibility/        # Visibility culling
├── Services/              # Business services
│   ├── Brushes/           # Brush lookup services
│   ├── Map/               # Map operations
│   ├── Preview/           # Preview providers
│   └── Selection/         # Selection services
├── UI/                    # ImGui interface
│   ├── Core/              # UI core (themes)
│   ├── Dialogs/           # Dialog windows
│   ├── DTOs/              # Data transfer objects
│   ├── Map/               # Map panel
│   ├── Panels/            # Dockable panels
│   ├── Ribbon/            # Ribbon toolbar
│   ├── Utils/             # UI utilities
│   ├── Widgets/           # Reusable widgets
│   └── Windows/           # Floating windows
├── Utils/                 # General utilities
├── ext/                   # External dependencies
├── sample_data/           # Sample configuration files
└── shaders/               # GLSL shader files
```

### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes | PascalCase | `ChunkedMap`, `TileRenderer` |
| Functions | camelCase | `loadMap()`, `getTile()` |
| Member Variables | snake_case_ (trailing underscore) | `sprite_manager_` |
| Constants | UPPER_SNAKE_CASE | `CHUNK_SIZE`, `MAX_FLOORS` |
| Namespaces | PascalCase | `MapEditor::Domain` |
| Files | PascalCase | `ChunkedMap.cpp`, `ChunkedMap.h` |

### Extension Points

**Adding a New Brush Type:**

1. Create class in `Brushes/Types/` inheriting from `IBrush`
2. Implement `apply()`, `canDraw()`, and metadata methods
3. Register in `BrushRegistry`
4. Add preview provider in `Services/Preview/`

**Adding a New Overlay:**

1. Create class in `Rendering/Overlays/` implementing `IOverlayRenderer`
2. Implement `render()` method
3. Register in `OverlayManager`
4. Add toggle in `ViewSettings`

**Adding a New File Format:**

1. Create reader/writer in `IO/`
2. Follow existing patterns (OtbmReader, etc.)
3. Wire into `MapLoadingService` or `MapSavingService`

**Adding a New Service:**

1. Create class in `Services/`
2. Use dependency injection via constructor
3. Wire in `Application::initializeServices()`
4. Document service responsibilities

---

## Contributing Guide

### Development Environment

1. **Clone with submodules:**
   ```bash
   git clone --recursive https://github.com/karolak6612/ImguiMapEditor.git
   ```

2. **IDE Setup:**
   - **Visual Studio 2022**: Open the folder, CMake integration automatic
   - **VS Code**: Install C++ and CMake extensions
   - **CLion**: Import CMake project

3. **Run tests:**
   ```bash
   cmake --build build --target run_tests
   ```

### Code Style Guidelines

> [!IMPORTANT]
> Read `AGENTS.MD` for complete style guidelines!

**Core Rules:**
- **C++20 standard**: Use modern features (concepts, ranges, etc.)
- **RAII everywhere**: No manual resource management
- **Const-correctness**: Mark methods `const` when possible
- **No globals**: Use dependency injection
- **One class per file**: Unless tightly coupled helpers
- **Max 120 chars**: Per line - soft rule
- **Max 50 lines**: Per function (refactor if longer) - soft rule

**Before submitting code:**
```
☐ No duplicate code
☐ Nothing in Application.cpp except wiring
☐ Code in correct module
☐ Existing utilities reused
☐ Headers properly organized
☐ RAII for OpenGL objects
☐ Clear ownership semantics
```

### Git Workflow

1. **Fork** the repository
2. **Create a feature branch**: `feature/my-feature`
3. **Make small, focused commits**
4. **Write descriptive commit messages**
5. **Open a Pull Request** against `main`
6. **Address review feedback**

**Commit Message Format:**
```
type(scope): Short description

Longer explanation if needed.

Fixes #123
```

Types: `feat`, `fix`, `refactor`, `docs`, `style`, `test`, `chore`

---

## Design Decisions

### Why Chunked Map Representation?

**Problem**: Tibia maps can be up to 65536x65536 tiles × 16 floors. A flat array would require 68 billion tile slots.

**Solution**: Chunk-based spatial index with 32x32 tile chunks.

**Benefits:**
- **Memory efficiency**: Only allocate chunks with content
- **Cache efficiency**: Tiles in a chunk are contiguous in memory
- **Culling**: Skip entire chunks outside viewport
- **Dirty tracking**: Invalidate render state per-chunk

### Why Modern OpenGL?

**Problem**: wxWidgets/GDI rendering in RME was CPU-bound and slow on large viewports.

**Solution**: OpenGL 3.3+ with modern techniques.

**Benefits:**
- **GPU acceleration**: Leverages dedicated graphics hardware
- **Batching**: Thousands of sprites in few draw calls
- **Shader-based**: Flexible effects (lighting, tinting)
- **Cross-platform**: Consistent behavior across OSes

### Why ImGui?

**Problem**: wxWidgets is verbose, platform-inconsistent, and slow to develop with.

**Solution**: Dear ImGui immediate-mode GUI.

**Benefits:**
- **Rapid iteration**: Change UI in real-time
- **Consistent**: Identical across all platforms
- **Docking**: Flexible panel layouts out of the box
- **Performance**: Minimal overhead

### Why Service-Oriented Architecture?

**Problem**: Monolithic code becomes unmaintainable.

**Solution**: Services with clear responsibilities and dependency injection.

**Benefits:**
- **Testability**: Mock services in tests
- **Decoupling**: Changes isolated to services
- **Clarity**: Easy to find functionality
- **Reusability**: Services can be shared

---

## FAQ

**Q: Is this a replacement for Remere's Map Editor?**

A: No. Its an experimental concept which is far from beeing a fully functional map editor.

**Q: Can I open my existing RME maps?**

A: Yes! The OTBM format is fully supported for reading, and partially for writing. 

**Q: Which client versions are supported?**

A: Tibia 7.10 through 10.10+. See `clients.json` for the complete list.

**Q: Will this work on Linux/macOS?**

A: The codebase is cross-platform, but testing has focused on Windows. Community testing on other platforms is welcome.

**Q: How can I contribute?**

A: Fork the repo, make changes, and open a Pull Request. See the Contributing Guide above.

---

## Credits & Acknowledgments

### Core Team

- Project Lead: [@karolak6612](https://github.com/karolak6612)
- Contributors: See [CONTRIBUTORS.md](CONTRIBUTORS.md)

### Special Thanks

- **Remere's Map Editor, Object Builder, Item Editor, Otclient/V8** developers for the foundational work
- **Open Tibia** community for format specifications
- **Dear ImGui** by Omar Cornut
- All open-source library authors

### Third-Party Libraries

See [Technology Stack](#technology-stack) for the complete list.

---

## License

This project is licensed under the **AGPL v3** - see the [LICENSE](LICENSE) file for details.

### Third-Party Licenses

This project includes code and assets from various open-source projects, each with their own licenses. See individual library documentation for details.

### Tibia Disclaimer

> [!CAUTION]
> Tibia is a registered trademark of CipSoft GmbH. This project is not affiliated with, endorsed by, or connected to CipSoft GmbH in any way. This is a community tool for Open Tibia servers.

---

<p align="center">
  <strong>Built with ❤️ by the Open Source Community</strong>
</p>

<p align="center">
  <a href="#top">⬆️ Back to Top</a>
</p>

