#pragma once

#include "components/parameter/ParameterManager.h"
#include "components/parameter/ParameterChangeEvent.h"
#include <unordered_map>
#include <functional>
#include <mutex>
#include <atomic>

namespace UI {

/**
 * @brief Maps MIDI controls to UI actions via parameter system
 * 
 * **Strategy:** MIDI CC → Virtual UI Parameter → UI Observer → UI Action
 * 
 * This maintains thread safety by using the existing parameter system
 * to route MIDI events to UI actions without direct thread communication.
 * 
 * **Usage:**
 * ```cpp
 * auto& bridge = MidiUIBridge::getInstance();
 * bridge.mapMidiToUI(1, 16, UIParameterID::UI_FOCUS_TRACK);
 * // Now MIDI CC 16 on channel 1 changes UI track focus
 * ```
 */
class MidiUIBridge {
public:
    /**
     * @brief Virtual UI parameter IDs for MIDI control
     * 
     * These are "virtual parameters" that don't control hardware but
     * trigger UI actions when changed via MIDI.
     */
    enum class UIParameterID : uint32_t {
        // Navigation Controls (CC 16-25)
        UI_FOCUS_TRACK = 20000,        ///< Change UI track focus (CC 16)
        UI_FOCUS_STEP = 20001,         ///< Change UI step focus (CC 17)
        UI_PAGE_SELECT = 20002,        ///< Switch UI pages/tabs (CC 18)
        UI_ZOOM_LEVEL = 20003,         ///< Pattern zoom level (CC 19)
        UI_SCROLL_HORIZONTAL = 20004,  ///< Horizontal scroll (CC 20)
        UI_SCROLL_VERTICAL = 20005,    ///< Vertical scroll (CC 21)
        
        // Display Controls (CC 26-35)
        UI_BRIGHTNESS = 20010,         ///< UI brightness (CC 26)
        UI_CONTRAST = 20011,           ///< UI contrast (CC 27)
        UI_COLOR_SCHEME = 20012,       ///< Color scheme selection (CC 28)
        UI_ANIMATION_SPEED = 20013,    ///< UI animation speed (CC 29)
        
        // Menu Controls (CC 36-45)
        UI_MENU_NAVIGATE = 20020,      ///< Menu navigation (CC 36)
        UI_MENU_SELECT = 20021,        ///< Menu selection (CC 37)
        UI_MENU_BACK = 20022,          ///< Menu back (CC 38)
        UI_MENU_HOME = 20023,          ///< Menu home (CC 39)
        
        // Modal Controls (CC 46-55)
        UI_PARAM_EDIT_MODE = 20030,    ///< Parameter editing mode (CC 46)
        UI_SAVE_DIALOG = 20031,        ///< Open save dialog (CC 47)
        UI_LOAD_DIALOG = 20032,        ///< Open load dialog (CC 48)
        UI_SETTINGS_DIALOG = 20033,    ///< Open settings dialog (CC 49)
        
        // Quick Actions (CC 56-59)
        UI_QUICK_SAVE = 20040,         ///< Quick save current state (CC 56)
        UI_QUICK_LOAD = 20041,         ///< Quick load last state (CC 57)
        UI_UNDO_UI_ACTION = 20042,     ///< Undo last UI action (CC 58)
        UI_REDO_UI_ACTION = 20043      ///< Redo last UI action (CC 59)
    };
    
    /**
     * @brief UI action function type
     * 
     * Functions that implement specific UI actions.
     * Called on main thread with normalized MIDI value (0.0-1.0).
     */
    using UIActionFunction = std::function<void(float)>;
    
    static MidiUIBridge& getInstance();
    
    /**
     * @brief Initialize with parameter manager
     * @param param_manager Parameter manager for receiving MIDI events
     */
    void initialize(std::shared_ptr<Parameters::ParameterManager> param_manager);
    
    /**
     * @brief Shutdown and cleanup
     */
    void shutdown();
    
    /**
     * @brief Process parameter change events (called by parameter manager)
     * @param event Parameter change event
     * @note This replaces the TypedObserver interface
     */
    void onParameterChanged(const Parameters::ParameterChangeEvent& event);
    
    // =============================================================================
    // MIDI → UI MAPPING
    // =============================================================================
    
    /**
     * @brief Map MIDI CC to UI action
     * @param channel MIDI channel (1-16)
     * @param cc MIDI CC (0-127)
     * @param ui_param UI parameter to control
     * @note Thread-safe, registers virtual parameter with parameter manager
     */
    void mapMidiToUI(uint8_t channel, uint8_t cc, UIParameterID ui_param);
    
    /**
     * @brief Remove MIDI → UI mapping
     * @param channel MIDI channel (1-16)
     * @param cc MIDI CC (0-127)
     */
    void removeMidiUIMapping(uint8_t channel, uint8_t cc);
    
    /**
     * @brief Register custom UI action function
     * @param ui_param UI parameter ID
     * @param action Function to execute for this UI action
     * @note Allows custom UI actions beyond built-in ones
     */
    void registerUIAction(UIParameterID ui_param, UIActionFunction action);
    
    // =============================================================================
    // BUILT-IN UI ACTIONS
    // =============================================================================
    
    /**
     * @brief Set UI manager for built-in actions
     * @param ui_manager Sequencer UI manager instance
     * @note Required for built-in UI actions to work
     */
    void setUIManager(std::shared_ptr<class SequencerUIManager> ui_manager);
    
    /**
     * @brief Setup default MIDI → UI mappings
     * @note Creates standard mappings for common UI actions
     */
    void setupDefaultUIMappings();
    
    // =============================================================================
    // OBSERVER INTERFACE (Simplified)
    // =============================================================================
    
    /**
     * @brief Check if parameter is a UI parameter
     * @param param_id Parameter ID to check
     * @return true if this is a UI parameter that should be handled by bridge
     */
    bool isUIParameter(Parameters::ParameterID param_id) const;
    
    // =============================================================================
    // DEBUGGING AND STATISTICS
    // =============================================================================
    
    /**
     * @brief Get number of active UI mappings
     * @return Count of MIDI CC → UI parameter mappings
     */
    size_t getActiveMappingCount() const;
    
    /**
     * @brief Export current UI mappings as text
     * @return String description of all UI mappings
     */
    std::string exportUIMappings() const;
    
    /**
     * @brief Print debugging information about UI mappings
     */
    void printUIMappingStatistics() const;

private:
    MidiUIBridge() = default;
    ~MidiUIBridge() = default;
    
    // =============================================================================
    // INTERNAL STATE
    // =============================================================================
    
    std::shared_ptr<Parameters::ParameterManager> param_manager_;
    std::shared_ptr<class SequencerUIManager> ui_manager_;
    std::atomic<bool> initialized_{false};
    
    // UI action registry
    std::unordered_map<UIParameterID, UIActionFunction> ui_actions_;
    mutable std::mutex ui_actions_mutex_;
    
    // Mapping storage: Parameter ID → UI Parameter ID
    std::unordered_map<Parameters::ParameterID, UIParameterID> ui_parameter_mappings_;
    mutable std::mutex mappings_mutex_;
    
    // Statistics
    std::atomic<uint64_t> ui_actions_executed_{0};
    std::atomic<uint64_t> ui_actions_queued_{0};
    
    // =============================================================================
    // INTERNAL METHODS
    // =============================================================================
    
    /**
     * @brief Process UI action on main thread
     * @param ui_param UI parameter that changed
     * @param value Normalized value (0.0-1.0)
     */
    void processUIAction(UIParameterID ui_param, float value);
    
    /**
     * @brief Register built-in UI actions
     */
    void registerBuiltInUIActions();
    
    /**
     * @brief Generate virtual parameter ID for UI parameter
     * @param ui_param UI parameter
     * @return Virtual parameter ID for parameter system
     */
    Parameters::ParameterID getVirtualParameterID(UIParameterID ui_param) const;
    
    /**
     * @brief Convert MIDI CC mapping to virtual parameter ID
     * @param channel MIDI channel (1-16)
     * @param cc MIDI CC (0-127)
     * @return Virtual parameter ID
     */
    Parameters::ParameterID getMidiUIParameterID(uint8_t channel, uint8_t cc) const;
    
    // =============================================================================
    // BUILT-IN UI ACTION IMPLEMENTATIONS
    // =============================================================================
    
    void handleTrackFocus(float value);
    void handleStepFocus(float value);
    void handlePageSelect(float value);
    void handleZoomLevel(float value);
    void handleScrollHorizontal(float value);
    void handleScrollVertical(float value);
    void handleBrightness(float value);
    void handleContrast(float value);
    void handleColorScheme(float value);
    void handleAnimationSpeed(float value);
    void handleMenuNavigate(float value);
    void handleMenuSelect(float value);
    void handleMenuBack(float value);
    void handleMenuHome(float value);
    void handleParamEditMode(float value);
    void handleSaveDialog(float value);
    void handleLoadDialog(float value);
    void handleSettingsDialog(float value);
    void handleQuickSave(float value);
    void handleQuickLoad(float value);
    void handleUndoUIAction(float value);
    void handleRedoUIAction(float value);
};

} // namespace UI
