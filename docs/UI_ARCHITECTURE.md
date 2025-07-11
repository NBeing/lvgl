# UI Architecture Overview - LVGL Synthesizer

## High-Level Architecture

The LVGL Synthesizer uses a **hierarchical tab-based UI system** with three main layers:

```
SynthApp (Main Application)
    ↓
WindowManager (Tab Container)  
    ↓
Tabs (Individual Screens)
    ↓
Controls (Dials, Buttons, etc.)
```

## Core Components

### 1. **SynthApp** - Main Application Controller
- **Location**: `src/components/app/SynthApp.cpp`
- **Purpose**: Top-level application that initializes everything
- **Key Responsibilities**:
  - Initialize LVGL and hardwabuildre 
  - Create parameter system (ParameterBinder, CommandManager)
  - Create and configure WindowManager
  - Set up MIDI handling

```cppxcompile
// Initialization flow in SynthApp::setup()
void SynthApp::setup() {
    // 1. Initialize parameter system
    parameter_binder_ = std::make_unique<ParameterBinder>();
    command_manager_ = std::make_unique<CommandManager>();
    
    // 2. Load synthesizer definitions
    parameter_binder_->loadSynthDefinition("hydrasynth");
    
    // 3. Initialize hardware and UI
    initHardware();
    initWindowManager();  // Creates tabs
}
```

### 2. **WindowManager** - Tab Container System
- **Location**: `src/components/ui/WindowManager.cpp`
- **Purpose**: Manages multiple tabs and handles tab switching
- **Key Features**:
  - Tab navigation (Previous/Next buttons)
  - Tab activation/deactivation lifecycle
  - Observer pattern for tab change notifications

```cpp
// WindowManager manages tab lifecycle
void WindowManager::showTab(const std::string& name) {
    if (current_tab_) {
        current_tab_->onDeactivated();  // Clean up old tab
    }
    
    auto it = tabs_.find(name);
    if (it != tabs_.end()) {
        current_tab_ = it->second.get();
        current_tab_->onActivated();    // Initialize new tab
        current_tab_->create(tab_container_);  // Create UI
    }
}
```

### 3. **Tab Base Class** - Individual Screen Interface
- **Location**: `src/components/ui/Tab.h`
- **Purpose**: Abstract base for all tab implementations
- **Key Methods**:
  - `create(parent)` - Build the tab's UI
  - `onActivated()` - Called when tab becomes visible
  - `onDeactivated()` - Called when tab is hidden
  - `update()` - Periodic updates

### 4. **Concrete Tabs** - Actual Screen Implementations

#### **MainControlTab** - Primary Synthesizer Controls
- **Location**: `src/components/ui/MainControlTab.cpp`
- **Purpose**: Main synthesizer interface with dials and undo/redo
- **Components**:
  - 6 Parameter dials (Cutoff, Resonance, Volume, Attack, Release, LFO Rate)
  - 3 Toggle buttons (Filter, LFO Sync, Trigger)
  - Undo/Redo command system
  - Status display

```cpp
// MainControlTab creates three sections
void MainControlTab::create(lv_obj_t* parent) {
    createDialsSection(container_);     // Grid of 6 dials
    createButtonsSection(container_);   // Row of 3 buttons  
    createStatusSection(container_);    // Status + Undo/Redo
    bindParameters();                   // Connect to parameter system
}
```

#### **HelloTab & WorldTab** - Simple Demo Tabs
- **Purpose**: Demonstrate tab switching and basic UI

## Instantiation Flow

### 1. **Application Startup** (main.cpp)
```cpp
SynthApp app;           // Create main application
app.setup();            // Initialize everything
while(true) {
    app.loop();         // Main event loop
}
```

### 2. **WindowManager Creation** (SynthApp::initWindowManager)
```cpp
void SynthApp::initWindowManager() {
    // Create window manager with navigation
    window_manager_ = std::make_unique<WindowManager>(screen);
    
    // Create all tabs
    createTabs();
    
    // Show first tab
    window_manager_->showTab("Main");
}
```

### 3. **Tab Creation** (SynthApp::createTabs)
```cpp
void SynthApp::createTabs() {
    // Create tabs with dependencies injected
    main_tab_ = std::make_unique<MainControlTab>(
        parameter_binder_.get(), 
        command_manager_.get(), 
        midi_handler_.get()
    );
    
    // Register tabs with window manager
    window_manager_->addTab(std::move(main_tab_));
    window_manager_->addTab(std::move(hello_tab_));
    window_manager_->addTab(std::move(world_tab_));
}
```

### 4. **Tab UI Creation** (Lazy Loading)
- **Tabs are created** when first activated
- **UI elements built** in `tab->create(parent)`
- **Parameters bound** to controls
- **Event callbacks** registered

## Layout Management

### **LayoutManager** - UI Constants and Sizing
- **Location**: `src/components/layout/LayoutManager.cpp`
- **Purpose**: Centralized UI configuration
- **Provides**:
  - Button dimensions
  - Spacing constants
  - Color schemes
  - Font selections

```cpp
// Layout configuration used throughout UI
const auto& layout = LayoutManager::getConfig();
lv_obj_set_size(button, layout.button_width, layout.button_height);
```

### **Responsive Layout Patterns**

#### **Grid Layout** (Dials)
```cpp
// 3x2 grid for parameter dials
static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
lv_obj_set_grid_dsc_array(container, col_dsc, row_dsc);
```

#### **Flex Layout** (Buttons)
```cpp
// Horizontal row of buttons with center alignment
lv_obj_set_layout(container, LV_LAYOUT_FLEX);
lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
```

## Key Design Patterns

### **Dependency Injection**
- Tabs receive dependencies via constructor (ParameterBinder, CommandManager, MidiHandler)
- Enables testing and modularity

### **Observer Pattern**
- WindowManager notifies observers of tab changes
- Parameters notify controls of value changes
- Commands notify UI of undo/redo state changes

### **Command Pattern**
- All parameter changes create Command objects
- Enables undo/redo functionality
- Separates UI actions from business logic

### **Factory Pattern**
- ParameterFactory creates synthesizer-specific parameter sets
- Supports multiple synthesizer models

## Data Flow

### **Parameter Change Flow**
```
User turns dial 
    ↓
DialControl detects change
    ↓
Value changed callback triggered
    ↓
MIDI sent to synthesizer + SetParameterCommand created
    ↓
Command executed through CommandManager
    ↓
Parameter value updated + UI refreshed
```

### **Undo/Redo Flow**
```
User clicks UNDO button
    ↓
MainControlTab::handleUndo()
    ↓
CommandManager::undo()
    ↓
Command::undo() restores old parameter value
    ↓
Parameter observers update UI controls
```

## Summary

The UI architecture is **modular and extensible**:
- **SynthApp** orchestrates everything
- **WindowManager** handles tab lifecycle 
- **Tabs** implement specific functionality
- **LayoutManager** provides consistent styling
- **Dependency injection** enables clean separation of concerns
- **Command pattern** provides undo/redo and MIDI integration

This design makes it easy to add new tabs, modify layouts, and extend functionality while maintaining clean code organization.
