#include "components/ui/MainControlTab.h"
#include "components/ui/UndoRedoPanel.h"
#include "components/ui/StatusInfoPanel.h"
#include "components/ui/ParameterDialsGrid.h"
#include "components/ui/ControlButtonsRow.h"
#include "components/parameter/ParameterBinder.h"
#include "components/parameter/CommandManager.h"
#include "components/parameter/Parameter.h"
#include "hardware/MidiHandler.h"
#include "components/midi/UnifiedMidiManager.h"
#include "components/parameter/Command.h"  // For SetParameterCommand
#include "Constants.h"
#include "components/ui/ContainerFactory.h"
#include <iostream>

MainControlTab::MainControlTab(ParameterBinder* param_binder, CommandManager* cmd_manager, MidiHandler* midi_handler)
    : Tab("Main")
    , parameter_binder_(param_binder)
    , command_manager_(cmd_manager)
    , midi_handler_(midi_handler)
    , dials_container_(nullptr)
    , buttons_container_(nullptr)
    , status_container_(nullptr)
{
}

void MainControlTab::create(lv_obj_t* parent) {
    if (container_) return;  // Already created

    // Create main container for this tab using mergeOptions
    UI::ContainerOptions mainOverrides = {
        .parent = parent,
        .width_pct = 98,
        .height_pct = 98,
        .align = LV_ALIGN_TOP_LEFT,
        .x_offset = 0,
        .y_offset = 0,
        .bg_color = lv_color_hex(SynthConstants::Color::BG),
        .bg_opa = LV_OPA_COVER,
        .border_width = 0,
        .pad_all = 2,
        .use_bg_color = true,
        .use_bg_opa = true
    };
    container_ = UI::createContainer(UI::mergeOptions(UI::DefaultContainer, mainOverrides));

    setContainer(container_);

    setupComponents();

    std::cout << "MainControlTab created with modular components" << std::endl;
}

void MainControlTab::setupComponents() {
    // Dials container: align to parent (top mid)
    UI::ContainerOptions dialsOverrides = {
        .parent = container_,
        .width_pct = 98,
        .height_pct = 60,
        .align = LV_ALIGN_TOP_MID,
        .x_offset = 0,
        .y_offset = 0,
        .bg_color = {},
        .bg_opa = LV_OPA_TRANSP,
        .border_width = 0,
        .pad_all = 2,
        .use_bg_color = false,
        .use_bg_opa = true
    };
    dials_container_ = UI::createContainer(UI::mergeOptions(UI::DefaultContainer, dialsOverrides));

    // Buttons container: align to dials_container_
    UI::ContainerOptions buttonsOverrides = {
        .parent = container_,
        .width_pct = 98,
        .height_pct = 20,
        .align = LV_ALIGN_TOP_LEFT, // temp, will realign below
        .x_offset = 0,
        .y_offset = 0,
        .bg_color = {},
        .bg_opa = LV_OPA_TRANSP,
        .border_width = 0,
        .pad_all = 2,
        .use_bg_color = false,
        .use_bg_opa = true
    };
    buttons_container_ = UI::createContainer(UI::mergeOptions(UI::DefaultContainer, buttonsOverrides));
    lv_obj_align_to(buttons_container_, dials_container_, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

    // Status container: align to buttons_container_
    UI::ContainerOptions statusOverrides = {
        .parent = container_,
        .width_pct = 98,
        .height_pct = 20,
        .align = LV_ALIGN_TOP_LEFT, // temp, will realign below
        .x_offset = 0,
        .y_offset = 0,
        .bg_color = {},
        .bg_opa = LV_OPA_COVER,
        .border_width = -1,
        .pad_all = -1,
        .use_bg_color = false,
        .use_bg_opa = false
    };
    status_container_ = UI::createContainer(UI::mergeOptions(UI::DefaultContainer, statusOverrides));
    lv_obj_align_to(status_container_, buttons_container_, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

    // Create components
    dials_grid_ = std::make_unique<ParameterDialsGrid>(parameter_binder_);
    buttons_row_ = std::make_unique<ControlButtonsRow>(parameter_binder_);
    undo_redo_panel_ = std::make_unique<UndoRedoPanel>(command_manager_);
    status_panel_ = std::make_unique<StatusInfoPanel>();

    // Setup component configurations
    setupDialDefinitions();
    setupButtonDefinitions();

    // Create UI components
    dials_grid_->create(dials_container_);
    buttons_row_->create(buttons_container_);
    status_panel_->create(status_container_);
    undo_redo_panel_->create(status_container_);

    // Set up callbacks
    dials_grid_->setParameterChangeCallback([this](uint8_t value, const Parameter* param) {
        onParameterChanged(value, param);
    });

    buttons_row_->setParameterChangeCallback([this](uint8_t value, const Parameter* param) {
        onParameterChanged(value, param);
    });

    undo_redo_panel_->setUndoCallback([this]() {
        onUndo();
    });

    undo_redo_panel_->setRedoCallback([this]() {
        onRedo();
    });

    // Position undo/redo panel in status container
    lv_obj_align(undo_redo_panel_->getContainer(), LV_ALIGN_RIGHT_MID, -10, 0);

    // Bind parameters
    dials_grid_->bindParameters();
    buttons_row_->bindParameters();

    // Initial status update
    updateStatusDisplay();
}

void MainControlTab::setupDialDefinitions() {
    std::vector<DialDefinition> dial_defs = {
        {"Filter 1 Cutoff", "Cutoff", SynthConstants::Color::DIAL_GREEN, 0, 0},
        {"Filter 1 Resonance", "Resonance", SynthConstants::Color::DIAL_ORANGE, 1, 0},
        {"Master Volume", "Volume", SynthConstants::Color::DIAL_BLUE, 2, 0},
        {"ENV 1 Attack", "Attack", SynthConstants::Color::DIAL_MAGENTA, 0, 1},
        {"ENV 1 Release", "Release", SynthConstants::Color::DIAL_YELLOW_GREEN, 1, 1},
        {"LFO 1 Rate", "LFO Rate", SynthConstants::Color::DIAL_PINK, 2, 1}
    };
    dials_grid_->setDialDefinitions(dial_defs);
}

void MainControlTab::setupButtonDefinitions() {
    std::vector<ButtonDefinition> button_defs = {
        {"Filter Enable", "FILTER", ButtonDefinition::Mode::TOGGLE},
        {"LFO Sync", "LFO SYNC", ButtonDefinition::Mode::TOGGLE},
        {"Trigger", "TRIGGER", ButtonDefinition::Mode::TRIGGER}
    };
    buttons_row_->setButtonDefinitions(button_defs);
}

void MainControlTab::onParameterChanged(uint8_t value, const Parameter* param) {
    std::cout << "MainControlTab::onParameterChanged called with value: " << (int)value << std::endl;
    
    // Send MIDI output using UnifiedMidiManager (supports both USB and hardware MIDI)
    if (param) {
        std::cout << "Parameter is valid, CC: " << (int)param->getCCNumber() << std::endl;
        auto& unified_midi = UnifiedMidiManager::getInstance();
        std::cout << "Got UnifiedMidiManager instance" << std::endl;
        
        // Check overall status
        auto overall_status = unified_midi.getOverallStatus();
        std::cout << "UnifiedMidiManager overall status: " << (int)overall_status << std::endl;
        
        if (unified_midi.isConnected()) {
            std::cout << "UnifiedMidiManager is connected - sending MIDI" << std::endl;
            unified_midi.sendControlChange(1, param->getCCNumber(), value);
            std::cout << "MIDI CC sent: CC" << (int)param->getCCNumber() << " = " << (int)value << std::endl;
        } else {
            std::cout << "UnifiedMidiManager is NOT connected!" << std::endl;
            
            // Check individual backends
            auto backends = unified_midi.getAvailableBackends();
            for (const auto& backend : backends) {
                std::cout << "Backend: " << backend.name << " - Status: " << (int)backend.status << std::endl;
            }
        }
    } else {
        std::cout << "Parameter is NULL!" << std::endl;
    }
    
    // Create command for undo/redo
    if (command_manager_) {
        auto command = std::make_unique<SetParameterCommand>(const_cast<Parameter*>(param), value);
        command_manager_->executeCommand(std::move(command));
        updateStatusDisplay();
    }
}

void MainControlTab::onUndo() {
    if (command_manager_ && command_manager_->canUndo()) {
        command_manager_->undo();
        updateStatusDisplay();
    }
}

void MainControlTab::onRedo() {
    if (command_manager_ && command_manager_->canRedo()) {
        command_manager_->redo();
        updateStatusDisplay();
    }
}

void MainControlTab::update() {
    updateStatusDisplay();
}

void MainControlTab::updateStatusDisplay() {
    if (!status_panel_ || !undo_redo_panel_ || !command_manager_) return;

    // Update undo/redo panel
    undo_redo_panel_->update();

    // Update status panel
    bool can_undo = command_manager_->canUndo();
    bool can_redo = command_manager_->canRedo();

    if (can_undo || can_redo) {
        std::string undo_desc = command_manager_->getUndoDescription();
        std::string redo_desc = command_manager_->getRedoDescription();
        status_panel_->setUndoRedoInfo(undo_desc, redo_desc);
    } else {
        status_panel_->setReadyMessage();
    }
}

void MainControlTab::onActivated() {
    std::cout << "MainControlTab activated" << std::endl;
}

void MainControlTab::onDeactivated() {
    std::cout << "MainControlTab deactivated" << std::endl;
}

