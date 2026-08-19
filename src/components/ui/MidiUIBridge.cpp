#include "MidiUIBridge.h"
#include "sequencer/SequencerUIManager.h"
#include <iostream>
#include <sstream>
#include <ctime>

namespace UI {

MidiUIBridge& MidiUIBridge::getInstance() {
    static MidiUIBridge instance;
    return instance;
}

void MidiUIBridge::initialize(std::shared_ptr<Parameters::ParameterManager> param_manager) {
    if (initialized_.load()) return;
    
    param_manager_ = param_manager;
    
    // Register built-in UI actions
    registerBuiltInUIActions();
    
    initialized_.store(true);
    std::cout << "[MidiUIBridge] ✨ MIDI → UI bridge initialized" << std::endl;
}

void MidiUIBridge::shutdown() {
    if (!initialized_.load()) return;
    
    std::lock_guard<std::mutex> lock(ui_actions_mutex_);
    ui_actions_.clear();
    
    std::lock_guard<std::mutex> lock2(mappings_mutex_);
    ui_parameter_mappings_.clear();
    
    initialized_.store(false);
    std::cout << "[MidiUIBridge] 💤 MIDI → UI bridge shutdown" << std::endl;
}

void MidiUIBridge::mapMidiToUI(uint8_t channel, uint8_t cc, UIParameterID ui_param) {
    if (!param_manager_) return;
    
    // Create virtual parameter ID for this UI parameter
    auto virtual_param_id = getVirtualParameterID(ui_param);
    
    // Map MIDI CC to virtual parameter in parameter manager
    param_manager_->assignMidiCC(virtual_param_id, channel, cc);
    
    // Store our UI mapping
    {
        std::lock_guard<std::mutex> lock(mappings_mutex_);
        ui_parameter_mappings_[virtual_param_id] = ui_param;
    }
    
    std::cout << "[MidiUIBridge] 🎛️ Mapped MIDI CC" << (int)cc 
              << " Ch" << (int)channel << " → UI action " << (int)ui_param << std::endl;
}

void MidiUIBridge::removeMidiUIMapping(uint8_t channel, uint8_t cc) {
    if (!param_manager_) return;
    
    // Get the virtual parameter ID that was mapped to this CC
    auto virtual_param_id = getMidiUIParameterID(channel, cc);
    
    if (virtual_param_id != 0) {
        // Remove from parameter manager
        param_manager_->removeMidiCC(channel, cc);
        
        // Remove from our mappings
        std::lock_guard<std::mutex> lock(mappings_mutex_);
        ui_parameter_mappings_.erase(virtual_param_id);
        
        std::cout << "[MidiUIBridge] ❌ Removed MIDI CC" << (int)cc 
                  << " Ch" << (int)channel << " UI mapping" << std::endl;
    }
}

void MidiUIBridge::registerUIAction(UIParameterID ui_param, UIActionFunction action) {
    std::lock_guard<std::mutex> lock(ui_actions_mutex_);
    ui_actions_[ui_param] = action;
}

void MidiUIBridge::setUIManager(std::shared_ptr<SequencerUIManager> ui_manager) {
    ui_manager_ = ui_manager;
    std::cout << "[MidiUIBridge] 🎯 UI manager connected for built-in actions" << std::endl;
}

void MidiUIBridge::setupDefaultUIMappings() {
    std::cout << "[MidiUIBridge] 🎚️ Setting up default UI mappings..." << std::endl;
    
    // Navigation controls on channel 1
    mapMidiToUI(1, 16, UIParameterID::UI_FOCUS_TRACK);      // CC 16 → Track focus
    mapMidiToUI(1, 17, UIParameterID::UI_FOCUS_STEP);       // CC 17 → Step focus
    mapMidiToUI(1, 18, UIParameterID::UI_PAGE_SELECT);      // CC 18 → Page select
    mapMidiToUI(1, 19, UIParameterID::UI_ZOOM_LEVEL);       // CC 19 → Zoom
    mapMidiToUI(1, 20, UIParameterID::UI_SCROLL_HORIZONTAL); // CC 20 → H-Scroll
    mapMidiToUI(1, 21, UIParameterID::UI_SCROLL_VERTICAL);   // CC 21 → V-Scroll
    
    // Display controls on channel 2
    mapMidiToUI(2, 26, UIParameterID::UI_BRIGHTNESS);       // CC 26 → Brightness
    mapMidiToUI(2, 27, UIParameterID::UI_CONTRAST);         // CC 27 → Contrast
    mapMidiToUI(2, 28, UIParameterID::UI_COLOR_SCHEME);     // CC 28 → Color scheme
    
    // Quick actions on channel 3
    mapMidiToUI(3, 56, UIParameterID::UI_QUICK_SAVE);       // CC 56 → Quick save
    mapMidiToUI(3, 57, UIParameterID::UI_QUICK_LOAD);       // CC 57 → Quick load
    mapMidiToUI(3, 58, UIParameterID::UI_UNDO_UI_ACTION);   // CC 58 → UI Undo
    mapMidiToUI(3, 59, UIParameterID::UI_REDO_UI_ACTION);   // CC 59 → UI Redo
    
    std::cout << "[MidiUIBridge] ✅ Default UI mappings configured" << std::endl;
}

void MidiUIBridge::onParameterChanged(const Parameters::ParameterChangeEvent& event) {
    // Check if this is a UI parameter change
    UIParameterID ui_param;
    {
        std::lock_guard<std::mutex> lock(mappings_mutex_);
        auto it = ui_parameter_mappings_.find(event.parameter_id);
        if (it == ui_parameter_mappings_.end()) {
            return; // Not a UI parameter
        }
        ui_param = it->second;
    }
    
    // Process UI action directly on main thread for now
    // In a full implementation, this would queue to UI thread
    processUIAction(ui_param, event.normalized_value);
    
    ui_actions_queued_.fetch_add(1);
}

bool MidiUIBridge::isUIParameter(Parameters::ParameterID param_id) const {
    std::lock_guard<std::mutex> lock(mappings_mutex_);
    return ui_parameter_mappings_.find(param_id) != ui_parameter_mappings_.end();
}

void MidiUIBridge::processUIAction(UIParameterID ui_param, float value) {
    UIActionFunction action;
    
    // Get action function
    {
        std::lock_guard<std::mutex> lock(ui_actions_mutex_);
        auto it = ui_actions_.find(ui_param);
        if (it == ui_actions_.end()) {
            std::cout << "[MidiUIBridge] ⚠️ No action registered for UI parameter " 
                      << (int)ui_param << std::endl;
            return;
        }
        action = it->second;
    }
    
    // Execute UI action on main thread
    try {
        action(value);
        ui_actions_executed_.fetch_add(1);
    }
    catch (const std::exception& e) {
        std::cout << "[MidiUIBridge] ❌ Error executing UI action: " << e.what() << std::endl;
    }
}

void MidiUIBridge::registerBuiltInUIActions() {
    // Navigation actions
    registerUIAction(UIParameterID::UI_FOCUS_TRACK, 
        [this](float value) { handleTrackFocus(value); });
    registerUIAction(UIParameterID::UI_FOCUS_STEP, 
        [this](float value) { handleStepFocus(value); });
    registerUIAction(UIParameterID::UI_PAGE_SELECT, 
        [this](float value) { handlePageSelect(value); });
    registerUIAction(UIParameterID::UI_ZOOM_LEVEL, 
        [this](float value) { handleZoomLevel(value); });
    
    // Display actions
    registerUIAction(UIParameterID::UI_BRIGHTNESS, 
        [this](float value) { handleBrightness(value); });
    registerUIAction(UIParameterID::UI_CONTRAST, 
        [this](float value) { handleContrast(value); });
    registerUIAction(UIParameterID::UI_COLOR_SCHEME, 
        [this](float value) { handleColorScheme(value); });
    
    // Quick actions
    registerUIAction(UIParameterID::UI_QUICK_SAVE, 
        [this](float value) { handleQuickSave(value); });
    registerUIAction(UIParameterID::UI_QUICK_LOAD, 
        [this](float value) { handleQuickLoad(value); });
    
    std::cout << "[MidiUIBridge] 🎯 Built-in UI actions registered" << std::endl;
}

Parameters::ParameterID MidiUIBridge::getVirtualParameterID(UIParameterID ui_param) const {
    // Convert UI parameter ID to virtual parameter ID
    return static_cast<Parameters::ParameterID>(ui_param);
}

Parameters::ParameterID MidiUIBridge::getMidiUIParameterID(uint8_t channel, uint8_t cc) const {
    if (!param_manager_) return 0;
    return param_manager_->getMidiMapping(channel, cc);
}

// =============================================================================
// BUILT-IN UI ACTION IMPLEMENTATIONS
// =============================================================================

void MidiUIBridge::handleTrackFocus(float value) {
    if (!ui_manager_) return;
    
    int track = static_cast<int>(value * 7.0f); // 0-7 tracks
    ui_manager_->setFocus(track, ui_manager_->getCurrentStep());
    
    std::cout << "[MidiUIBridge] 🎚️ Track focus → " << track << std::endl;
}

void MidiUIBridge::handleStepFocus(float value) {
    if (!ui_manager_) return;
    
    int step = static_cast<int>(value * 15.0f); // 0-15 steps
    ui_manager_->setFocus(ui_manager_->getCurrentTrack(), step);
    
    std::cout << "[MidiUIBridge] 📍 Step focus → " << step << std::endl;
}

void MidiUIBridge::handlePageSelect(float value) {
    if (!ui_manager_) return;
    
    // Convert to pattern number (0-15)
    int pattern = static_cast<int>(value * 15.0f);
    ui_manager_->switchToPattern(pattern);
    
    std::cout << "[MidiUIBridge] 📄 Page/Pattern → " << pattern << std::endl;
}

void MidiUIBridge::handleZoomLevel(float value) {
    // Zoom level implementation would depend on UI manager capabilities
    float zoom = 0.5f + (value * 2.0f); // 0.5x to 2.5x zoom
    std::cout << "[MidiUIBridge] 🔍 Zoom level → " << zoom << "x" << std::endl;
    
    // Implementation would call ui_manager_->setZoomLevel(zoom) if available
}

void MidiUIBridge::handleBrightness(float value) {
    // Brightness control (0-100%)
    int brightness = static_cast<int>(value * 100.0f);
    std::cout << "[MidiUIBridge] 💡 Brightness → " << brightness << "%" << std::endl;
    
    // Implementation would adjust LVGL screen brightness
    // lv_disp_set_brightness(lv_disp_get_default(), brightness * 255 / 100);
}

void MidiUIBridge::handleContrast(float value) {
    // Contrast control (0-100%)
    int contrast = static_cast<int>(value * 100.0f);
    std::cout << "[MidiUIBridge] 🌗 Contrast → " << contrast << "%" << std::endl;
    
    // Implementation would adjust display contrast
}

void MidiUIBridge::handleColorScheme(float value) {
    // Color scheme selection (0-3 for 4 schemes)
    int scheme = static_cast<int>(value * 3.0f);
    const char* schemes[] = {"Dark", "Light", "Blue", "Green"};
    
    std::cout << "[MidiUIBridge] 🎨 Color scheme → " << schemes[scheme] << std::endl;
    
    // Implementation would switch UI color theme
}

void MidiUIBridge::handleQuickSave(float value) {
    if (value < 0.5f) return; // Only trigger on high values
    
    std::cout << "[MidiUIBridge] 💾 Quick save triggered" << std::endl;
    
    // Implementation would save current sequencer state
    // if (ui_manager_) ui_manager_->quickSave();
}

void MidiUIBridge::handleQuickLoad(float value) {
    if (value < 0.5f) return; // Only trigger on high values
    
    std::cout << "[MidiUIBridge] 📁 Quick load triggered" << std::endl;
    
    // Implementation would load last saved state
    // if (ui_manager_) ui_manager_->quickLoad();
}

// =============================================================================
// DEBUGGING AND STATISTICS
// =============================================================================

size_t MidiUIBridge::getActiveMappingCount() const {
    std::lock_guard<std::mutex> lock(mappings_mutex_);
    return ui_parameter_mappings_.size();
}

std::string MidiUIBridge::exportUIMappings() const {
    std::stringstream ss;
    ss << "# MIDI → UI Control Mappings\n";
    ss << "# Generated: " << std::time(nullptr) << "\n\n";
    
    std::lock_guard<std::mutex> lock(mappings_mutex_);
    
    for (const auto& [param_id, ui_param] : ui_parameter_mappings_) {
        // Would need to reverse-lookup MIDI CC from parameter ID
        ss << "Parameter " << param_id << " → UI Action " << (int)ui_param << "\n";
    }
    
    ss << "\n# Statistics:\n";
    ss << "# Active mappings: " << ui_parameter_mappings_.size() << "\n";
    ss << "# Actions executed: " << ui_actions_executed_.load() << "\n";
    ss << "# Actions queued: " << ui_actions_queued_.load() << "\n";
    
    return ss.str();
}

void MidiUIBridge::printUIMappingStatistics() const {
    std::cout << "\n🎛️ === MIDI → UI BRIDGE STATISTICS ===" << std::endl;
    std::cout << "Active mappings: " << getActiveMappingCount() << std::endl;
    std::cout << "UI actions executed: " << ui_actions_executed_.load() << std::endl;
    std::cout << "UI actions queued: " << ui_actions_queued_.load() << std::endl;
    std::cout << "Initialized: " << (initialized_.load() ? "Yes" : "No") << std::endl;
    std::cout << "UI Manager connected: " << (ui_manager_ ? "Yes" : "No") << std::endl;
    std::cout << "=======================================" << std::endl;
}

// Placeholder implementations for remaining built-in actions
void MidiUIBridge::handleScrollHorizontal(float value) { /* Implementation */ }
void MidiUIBridge::handleScrollVertical(float value) { /* Implementation */ }
void MidiUIBridge::handleAnimationSpeed(float value) { /* Implementation */ }
void MidiUIBridge::handleMenuNavigate(float value) { /* Implementation */ }
void MidiUIBridge::handleMenuSelect(float value) { /* Implementation */ }
void MidiUIBridge::handleMenuBack(float value) { /* Implementation */ }
void MidiUIBridge::handleMenuHome(float value) { /* Implementation */ }
void MidiUIBridge::handleParamEditMode(float value) { /* Implementation */ }
void MidiUIBridge::handleSaveDialog(float value) { /* Implementation */ }
void MidiUIBridge::handleLoadDialog(float value) { /* Implementation */ }
void MidiUIBridge::handleSettingsDialog(float value) { /* Implementation */ }
void MidiUIBridge::handleUndoUIAction(float value) { /* Implementation */ }
void MidiUIBridge::handleRedoUIAction(float value) { /* Implementation */ }

} // namespace UI
