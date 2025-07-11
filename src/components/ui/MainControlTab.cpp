#include "components/ui/MainControlTab.h"
#include "components/layout/LayoutManager.h"
#include "FontConfig.h"
#include "Constants.h"
#include <iostream>
#include "ButtonControl.h"
#include "Command.h"  // For SetParameterCommand

// Static callback data
static MainControlTab* main_tab_instance = nullptr;

MainControlTab::MainControlTab(ParameterBinder* param_binder, CommandManager* cmd_manager, MidiHandler* midi_handler)
    : Tab("Main")
    , parameter_binder_(param_binder)
    , command_manager_(cmd_manager)
    , midi_handler_(midi_handler)
    , dials_container_(nullptr)
    , buttons_container_(nullptr)
    , status_container_(nullptr)
    , undo_btn_(nullptr)
    , redo_btn_(nullptr)
    , status_label_(nullptr)
{
    main_tab_instance = this;  // For static callbacks
}

void MainControlTab::create(lv_obj_t* parent) {
    if (container_) return;  // Already created

    // Create main container for this tab
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(98), LV_PCT(98));  // 98% to prevent overflow
    lv_obj_set_pos(container_, 0, 0);
    lv_obj_set_style_bg_color(container_, lv_color_hex(SynthConstants::Color::BG), 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 2, 0);  // Minimal padding to prevent overflow

    setContainer(container_);

    // Create sections
    createDialsSection(container_);
    createButtonsSection(container_);
    createStatusSection(container_);

    // Bind parameters
    bindParameters();

    std::cout << "MainControlTab created" << std::endl;
}

void MainControlTab::createDialsSection(lv_obj_t* parent) {
    // Create container for parameter dials - reduced height to 60% to leave more room
    dials_container_ = lv_obj_create(parent);
    lv_obj_set_size(dials_container_, LV_PCT(98), LV_PCT(60));  // 98% width to prevent overflow
    lv_obj_align(dials_container_, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(dials_container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dials_container_, 0, 0);
    lv_obj_set_style_pad_all(dials_container_, 2, 0);  // Minimal padding

    // Set up grid layout for dials
    lv_obj_set_layout(dials_container_, LV_LAYOUT_GRID);
    
    // Define grid template (3 columns, 2 rows)
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(dials_container_, col_dsc, row_dsc);

    // Create dial controls
    cutoff_dial_ = std::make_shared<DialControl>(dials_container_, 0, 0);
    cutoff_dial_->setColor(lv_color_hex(SynthConstants::Color::DIAL_GREEN));
    lv_obj_set_grid_cell(cutoff_dial_->getObject(), LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    resonance_dial_ = std::make_shared<DialControl>(dials_container_, 0, 0);
    resonance_dial_->setColor(lv_color_hex(SynthConstants::Color::DIAL_ORANGE));
    lv_obj_set_grid_cell(resonance_dial_->getObject(), LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    volume_dial_ = std::make_shared<DialControl>(dials_container_, 0, 0);
    volume_dial_->setColor(lv_color_hex(SynthConstants::Color::DIAL_BLUE));
    lv_obj_set_grid_cell(volume_dial_->getObject(), LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    attack_dial_ = std::make_shared<DialControl>(dials_container_, 0, 0);
    attack_dial_->setColor(lv_color_hex(SynthConstants::Color::DIAL_MAGENTA));
    lv_obj_set_grid_cell(attack_dial_->getObject(), LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);

    release_dial_ = std::make_shared<DialControl>(dials_container_, 0, 0);
    release_dial_->setColor(lv_color_hex(SynthConstants::Color::DIAL_YELLOW_GREEN));
    lv_obj_set_grid_cell(release_dial_->getObject(), LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);

    lfo_rate_dial_ = std::make_shared<DialControl>(dials_container_, 0, 0);
    lfo_rate_dial_->setColor(lv_color_hex(SynthConstants::Color::DIAL_PINK));
    lv_obj_set_grid_cell(lfo_rate_dial_->getObject(), LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 1, 1);
}

void MainControlTab::createButtonsSection(lv_obj_t* parent) {
    // Create container for buttons - increased to 20% 
    buttons_container_ = lv_obj_create(parent);
    lv_obj_set_size(buttons_container_, LV_PCT(98), LV_PCT(20));  // 98% width to prevent overflow
    lv_obj_align_to(buttons_container_, dials_container_, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
    lv_obj_set_style_bg_opa(buttons_container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(buttons_container_, 0, 0);
    lv_obj_set_style_pad_all(buttons_container_, 2, 0);  // Minimal padding

    // Set up flex layout for buttons
    lv_obj_set_layout(buttons_container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(buttons_container_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(buttons_container_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(buttons_container_, 20, 0);

    const auto& layout = LayoutManager::getConfig();

    // Create button controls
    filter_enable_btn_ = std::make_shared<ButtonControl>(buttons_container_, 0, 0, layout.button_width, layout.button_height);
    filter_enable_btn_->setLabel("FILTER");
    filter_enable_btn_->setMode(ButtonControl::ButtonMode::TOGGLE);

    lfo_sync_btn_ = std::make_shared<ButtonControl>(buttons_container_, 0, 0, layout.button_width, layout.button_height);
    lfo_sync_btn_->setLabel("LFO SYNC");
    lfo_sync_btn_->setMode(ButtonControl::ButtonMode::TOGGLE);

    trigger_btn_ = std::make_shared<ButtonControl>(buttons_container_, 0, 0, layout.button_width, layout.button_height);
    trigger_btn_->setLabel("TRIGGER");
    trigger_btn_->setMode(ButtonControl::ButtonMode::TRIGGER);
}

void MainControlTab::createStatusSection(lv_obj_t* parent) {
    // Create container for status and undo/redo - increased to 20% for better visibility
    status_container_ = lv_obj_create(parent);
    lv_obj_set_size(status_container_, LV_PCT(98), LV_PCT(20));  // 98% width to prevent overflow
    lv_obj_align_to(status_container_, buttons_container_, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
    lv_obj_set_style_bg_color(status_container_, lv_color_hex(SynthConstants::Color::STATUS_BG), 0);
    lv_obj_set_style_border_color(status_container_, lv_color_hex(SynthConstants::Color::STATUS_BORDER), 0);
    lv_obj_set_style_border_width(status_container_, 1, 0);
    lv_obj_set_style_radius(status_container_, 4, 0);
    lv_obj_set_style_pad_all(status_container_, 2, 0);  // Minimal padding

    // Create status label
    status_label_ = lv_label_create(status_container_);
    lv_label_set_text(status_label_, SynthConstants::Text::STATUS_READY);
    lv_obj_set_style_text_color(status_label_, lv_color_hex(SynthConstants::Color::STATUS), 0);
    lv_obj_align(status_label_, LV_ALIGN_LEFT_MID, 10, 0);

    createUndoRedoControls(status_container_);
}

void MainControlTab::createUndoRedoControls(lv_obj_t* parent) {
    const auto& layout = LayoutManager::getConfig();

    // Create undo button using simple LVGL button like the working SynthApp version
    undo_btn_ = lv_btn_create(parent);
    lv_obj_set_size(undo_btn_, layout.button_width, layout.button_height);
    lv_obj_align(undo_btn_, LV_ALIGN_RIGHT_MID, -80, 0);
    
    // Style the undo button
    lv_obj_set_style_bg_color(undo_btn_, lv_color_hex(SynthConstants::Color::BTN_UNDO_BG), 0);
    lv_obj_set_style_border_color(undo_btn_, lv_color_hex(SynthConstants::Color::BTN_UNDO_BORDER), 0);
    lv_obj_set_style_border_width(undo_btn_, 2, 0);
    lv_obj_set_style_radius(undo_btn_, 4, 0);
    
    lv_obj_t* undo_label = lv_label_create(undo_btn_);
    lv_label_set_text(undo_label, "UNDO");
    lv_obj_center(undo_label);
    
    // Add event callback using lambda like the working version
    lv_obj_add_event_cb(undo_btn_, [](lv_event_t* e) {
        MainControlTab* tab = static_cast<MainControlTab*>(lv_event_get_user_data(e));
        tab->handleUndo();
    }, LV_EVENT_CLICKED, this);
    
    std::cout << "UNDO button created with working SynthApp-style callback" << std::endl;

    // Create redo button using simple LVGL button like the working SynthApp version  
    redo_btn_ = lv_btn_create(parent);
    lv_obj_set_size(redo_btn_, layout.button_width, layout.button_height);
    lv_obj_align(redo_btn_, LV_ALIGN_RIGHT_MID, -10, 0);
    
    // Style the redo button
    lv_obj_set_style_bg_color(redo_btn_, lv_color_hex(SynthConstants::Color::BTN_REDO_BG), 0);
    lv_obj_set_style_border_color(redo_btn_, lv_color_hex(SynthConstants::Color::BTN_REDO_BORDER), 0);
    lv_obj_set_style_border_width(redo_btn_, 2, 0);
    lv_obj_set_style_radius(redo_btn_, 4, 0);
    
    lv_obj_t* redo_label = lv_label_create(redo_btn_);
    lv_label_set_text(redo_label, "REDO");
    lv_obj_center(redo_label);
    
    // Add event callback using lambda like the working version
    lv_obj_add_event_cb(redo_btn_, [](lv_event_t* e) {
        MainControlTab* tab = static_cast<MainControlTab*>(lv_event_get_user_data(e));
        tab->handleRedo();
    }, LV_EVENT_CLICKED, this);
    
    std::cout << "REDO button created with working SynthApp-style callback" << std::endl;
}

void MainControlTab::bindParameters() {
    if (!parameter_binder_) {
        std::cout << "ERROR: parameter_binder_ is NULL!" << std::endl;
        return;
    }

    std::cout << "Binding parameters with command callbacks..." << std::endl;

    // Ensure parameters are loaded
    if (!parameter_binder_->loadSynthDefinition("hydrasynth")) {
        std::cout << "Failed to load Hydrasynth parameters!" << std::endl;
        return;
    }

    // Bind dial controls to parameters AND set up command creation
    auto cutoff_param = parameter_binder_->getParameter("Filter 1 Cutoff");
    if (cutoff_param) {
        cutoff_dial_->bindParameter(cutoff_param);
        
        // Set up value changed callback to create commands AND send MIDI
        cutoff_dial_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
            // Send MIDI output
            if (midi_handler_ && midi_handler_->isConnected() && param) {
                midi_handler_->sendControlChange(1, param->getCCNumber(), value);
            }
            
            // Create command for undo/redo
            if (command_manager_) {
                auto command = std::make_unique<SetParameterCommand>(const_cast<Parameter*>(param), value);
                command_manager_->executeCommand(std::move(command));
                updateStatusDisplay();
            }
        });
    }

    auto resonance_param = parameter_binder_->getParameter("Filter 1 Resonance");
    if (resonance_param) {
        resonance_dial_->bindParameter(resonance_param);
        resonance_dial_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
            if (midi_handler_ && midi_handler_->isConnected() && param) {
                midi_handler_->sendControlChange(1, param->getCCNumber(), value);
            }
            if (command_manager_) {
                auto command = std::make_unique<SetParameterCommand>(const_cast<Parameter*>(param), value);
                command_manager_->executeCommand(std::move(command));
                updateStatusDisplay();
            }
        });
    }

    auto volume_param = parameter_binder_->getParameter("Master Volume");
    if (volume_param) {
        volume_dial_->bindParameter(volume_param);
        volume_dial_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
            if (midi_handler_ && midi_handler_->isConnected() && param) {
                midi_handler_->sendControlChange(1, param->getCCNumber(), value);
            }
            if (command_manager_) {
                auto command = std::make_unique<SetParameterCommand>(const_cast<Parameter*>(param), value);
                command_manager_->executeCommand(std::move(command));
                updateStatusDisplay();
            }
        });
    }

    auto attack_param = parameter_binder_->getParameter("ENV 1 Attack");
    if (attack_param) {
        attack_dial_->bindParameter(attack_param);
        attack_dial_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
            if (midi_handler_ && midi_handler_->isConnected() && param) {
                midi_handler_->sendControlChange(1, param->getCCNumber(), value);
            }
            if (command_manager_) {
                auto command = std::make_unique<SetParameterCommand>(const_cast<Parameter*>(param), value);
                command_manager_->executeCommand(std::move(command));
                updateStatusDisplay();
            }
        });
    }

    auto release_param = parameter_binder_->getParameter("ENV 1 Release");
    if (release_param) {
        release_dial_->bindParameter(release_param);
        release_dial_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
            if (midi_handler_ && midi_handler_->isConnected() && param) {
                midi_handler_->sendControlChange(1, param->getCCNumber(), value);
            }
            if (command_manager_) {
                auto command = std::make_unique<SetParameterCommand>(const_cast<Parameter*>(param), value);
                command_manager_->executeCommand(std::move(command));
                updateStatusDisplay();
            }
        });
    }

    auto lfo_rate_param = parameter_binder_->getParameter("LFO 1 Rate");
    if (lfo_rate_param) {
        lfo_rate_dial_->bindParameter(lfo_rate_param);
        lfo_rate_dial_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
            if (midi_handler_ && midi_handler_->isConnected() && param) {
                midi_handler_->sendControlChange(1, param->getCCNumber(), value);
            }
            if (command_manager_) {
                auto command = std::make_unique<SetParameterCommand>(const_cast<Parameter*>(param), value);
                command_manager_->executeCommand(std::move(command));
                updateStatusDisplay();
            }
        });
    }

    // Bind button controls to parameters with command creation
    auto filter_enable_param = parameter_binder_->getParameter("Filter Enable");
    if (filter_enable_param) {
        filter_enable_btn_->bindParameter(filter_enable_param);
        filter_enable_btn_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
            if (midi_handler_ && midi_handler_->isConnected() && param) {
                midi_handler_->sendControlChange(1, param->getCCNumber(), value);
            }
            if (command_manager_) {
                auto command = std::make_unique<SetParameterCommand>(const_cast<Parameter*>(param), value);
                command_manager_->executeCommand(std::move(command));
                updateStatusDisplay();
            }
        });
    }

    auto lfo_sync_param = parameter_binder_->getParameter("LFO Sync");
    if (lfo_sync_param) {
        lfo_sync_btn_->bindParameter(lfo_sync_param);
        lfo_sync_btn_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
            if (midi_handler_ && midi_handler_->isConnected() && param) {
                midi_handler_->sendControlChange(1, param->getCCNumber(), value);
            }
            if (command_manager_) {
                auto command = std::make_unique<SetParameterCommand>(const_cast<Parameter*>(param), value);
                command_manager_->executeCommand(std::move(command));
                updateStatusDisplay();
            }
        });
    }

    auto trigger_param = parameter_binder_->getParameter("Trigger");
    if (trigger_param) {
        trigger_btn_->bindParameter(trigger_param);
        trigger_btn_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
            if (midi_handler_ && midi_handler_->isConnected() && param) {
                midi_handler_->sendControlChange(1, param->getCCNumber(), value);
            }
            if (command_manager_) {
                auto command = std::make_unique<SetParameterCommand>(const_cast<Parameter*>(param), value);
                command_manager_->executeCommand(std::move(command));
                updateStatusDisplay();
            }
        });
    }

    std::cout << "Parameters bound with command creation callbacks" << std::endl;
}

void MainControlTab::update() {
    updateStatusDisplay();
}

void MainControlTab::updateStatusDisplay() {
    if (!status_label_ || !command_manager_) return;

    // Update undo/redo button states
    bool can_undo = command_manager_->canUndo();
    bool can_redo = command_manager_->canRedo();

    // Update button visual states like the working version
    if (undo_btn_) {
        lv_obj_t* undo_label = lv_obj_get_child(undo_btn_, 0);
        if (undo_label) {
            lv_label_set_text(undo_label, can_undo ? "UNDO" : "UNDO-");
            // Update button background color to show state
            lv_obj_set_style_bg_color(undo_btn_, 
                can_undo ? lv_color_hex(0x005500) : lv_color_hex(SynthConstants::Color::BTN_UNDO_BG), 
                LV_STATE_DEFAULT);
        }
    }

    if (redo_btn_) {
        lv_obj_t* redo_label = lv_obj_get_child(redo_btn_, 0);
        if (redo_label) {
            lv_label_set_text(redo_label, can_redo ? "REDO" : "REDO-");
            // Update button background color to show state
            lv_obj_set_style_bg_color(redo_btn_, 
                can_redo ? lv_color_hex(0x550000) : lv_color_hex(SynthConstants::Color::BTN_REDO_BG), 
                LV_STATE_DEFAULT);
        }
    }

    // Update status text with command descriptions like the working version
    if (can_undo || can_redo) {
        std::string undo_desc = command_manager_->getUndoDescription();
        std::string redo_desc = command_manager_->getRedoDescription();
        
        char status_text[256];
        snprintf(status_text, sizeof(status_text), "U:%s | R:%s", 
                undo_desc.c_str(), redo_desc.c_str());
        lv_label_set_text(status_label_, status_text);
    } else {
        lv_label_set_text(status_label_, "Ready - Parameters loaded");
    }
}

std::shared_ptr<DialControl> MainControlTab::getDialControl(const std::string& name) const {
    if (name == "Cutoff") return cutoff_dial_;
    if (name == "Resonance") return resonance_dial_;
    if (name == "Volume") return volume_dial_;
    if (name == "Attack") return attack_dial_;
    if (name == "Release") return release_dial_;
    if (name == "LFO Rate") return lfo_rate_dial_;
    return nullptr;
}

std::shared_ptr<ButtonControl> MainControlTab::getButtonControl(const std::string& name) const {
    if (name == "Filter") return filter_enable_btn_;
    if (name == "LFO Sync") return lfo_sync_btn_;
    if (name == "Trigger") return trigger_btn_;
    return nullptr;
}

void MainControlTab::onActivated() {
    std::cout << "MainControlTab activated" << std::endl;
}

void MainControlTab::onDeactivated() {
    std::cout << "MainControlTab deactivated" << std::endl;
}

void MainControlTab::handleUndo() {
    if (command_manager_ && command_manager_->canUndo()) {
        command_manager_->undo();
        updateStatusDisplay();
    }
}

void MainControlTab::handleRedo() {
    if (command_manager_ && command_manager_->canRedo()) {
        command_manager_->redo();
        updateStatusDisplay();
    }
}
