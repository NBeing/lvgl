#pragma once

#include "components/ui/Window.h"
#include "components/controls/DialControl.h"
#include "components/controls/ButtonControl.h"
#include "components/parameter/ParameterBinder.h"
#include "components/parameter/CommandManager.h"
#include "hardware/MidiHandler.h"
#include <memory>
#include <vector>

/**
 * @brief Main synthesizer control tab containing parameter dials and buttons
 * 
 * This tab contains the core synthesizer controls that were previously
 * in SynthApp directly. It demonstrates how to organize controls within
 * the new tab-based architecture.
 */
class MainControlTab : public Tab {
public:
    MainControlTab(ParameterBinder* param_binder, CommandManager* cmd_manager, MidiHandler* midi_handler);
    virtual ~MainControlTab() = default;

    // Window interface
    void create(lv_obj_t* parent) override;
    void update() override;

    // Control access (for external interaction)
    std::shared_ptr<DialControl> getDialControl(const std::string& name) const;
    std::shared_ptr<ButtonControl> getButtonControl(const std::string& name) const;

protected:
    void onActivated() override;
    void onDeactivated() override;

private:
    // Dependencies
    ParameterBinder* parameter_binder_;
    CommandManager* command_manager_;
    MidiHandler* midi_handler_;

    // UI Layout containers
    lv_obj_t* dials_container_;
    lv_obj_t* buttons_container_;
    lv_obj_t* status_container_;

    // Parameter controls (moved from SynthApp)
    std::shared_ptr<DialControl> cutoff_dial_;
    std::shared_ptr<DialControl> resonance_dial_;
    std::shared_ptr<DialControl> volume_dial_;
    std::shared_ptr<DialControl> attack_dial_;
    std::shared_ptr<DialControl> release_dial_;
    std::shared_ptr<DialControl> lfo_rate_dial_;

    // Button controls
    std::shared_ptr<ButtonControl> filter_enable_btn_;
    std::shared_ptr<ButtonControl> lfo_sync_btn_;
    std::shared_ptr<ButtonControl> trigger_btn_;

    // Undo/Redo controls using direct LVGL buttons for better event handling
    lv_obj_t* undo_btn_;
    lv_obj_t* redo_btn_;
    lv_obj_t* status_label_;

    // Layout and creation helpers
    void createDialsSection(lv_obj_t* parent);
    void createButtonsSection(lv_obj_t* parent);
    void createStatusSection(lv_obj_t* parent);
    void createUndoRedoControls(lv_obj_t* parent);
    
    void bindParameters();
    void updateStatusDisplay();
    void layoutControls();

    // Event handlers (now using lambdas like working SynthApp version)
    void handleUndo();
    void handleRedo();
};
