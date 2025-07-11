#pragma once

#include "components/ui/Window.h"
#include "components/ui/UndoRedoPanel.h"
#include "components/ui/StatusInfoPanel.h" 
#include "components/ui/ParameterDialsGrid.h"
#include "components/ui/ControlButtonsRow.h"
#include <memory>
#include <string>

class ParameterBinder;
class CommandManager; 
class MidiHandler;
class Parameter;

/**
 * @brief Main synthesizer control tab containing parameter dials and buttons
 * 
 * This tab contains the core synthesizer controls organized into modular components.
 * It demonstrates clean separation of concerns and reusable UI components.
 */
class MainControlTab : public Tab {
public:
    MainControlTab(ParameterBinder* param_binder, CommandManager* cmd_manager, MidiHandler* midi_handler);
    virtual ~MainControlTab() = default;

    // Tab interface
    void create(lv_obj_t* parent) override;
    void update() override;

protected:
    void onActivated() override;
    void onDeactivated() override;

private:
    void setupComponents();
    void setupDialDefinitions();
    void setupButtonDefinitions();
    void onParameterChanged(uint8_t value, const Parameter* param);
    void onUndo();
    void onRedo();
    void updateStatusDisplay();

    // Dependencies
    ParameterBinder* parameter_binder_;
    CommandManager* command_manager_;
    MidiHandler* midi_handler_;

    // UI Components
    std::unique_ptr<ParameterDialsGrid> dials_grid_;
    std::unique_ptr<ControlButtonsRow> buttons_row_;
    std::unique_ptr<UndoRedoPanel> undo_redo_panel_;
    std::unique_ptr<StatusInfoPanel> status_panel_;

    // Layout containers
    lv_obj_t* dials_container_;
    lv_obj_t* buttons_container_;
    lv_obj_t* status_container_;
};
