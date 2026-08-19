#include "ParameterSelectionUI.h"
#include "components/layout/LayoutManager.h"
#include <iostream>
#include <sstream>
#include <algorithm>

// ============================================================================
// ParameterSelectionUI Implementation - Redesigned for 480x320 ESP32 Screen
// ============================================================================

ParameterSelectionUI::ParameterSelectionUI(SynthParameterManager* synth_param_manager,
                                         Parameters::ParameterManager* param_manager)
    : Tab("Parameters")
    , synth_param_manager_(synth_param_manager)
    , param_manager_(param_manager)
    , main_container_(nullptr)
    , search_container_(nullptr)
    , category_tabs_(nullptr)
    , parameter_list_(nullptr)
    , details_panel_(nullptr)
    , action_buttons_(nullptr)
    , search_input_(nullptr)
    , synth_dropdown_(nullptr)
    , favorites_btn_(nullptr)
    , recents_btn_(nullptr)
    , selected_parameter_(nullptr)
    , current_category_(ParameterCategory::UNKNOWN)
    , current_mode_(DisplayMode::ALL_PARAMETERS)
{
    std::cout << "[ParameterSelectionUI] Initialized for 480x320 8-bit design" << std::endl;
}

ParameterSelectionUI::~ParameterSelectionUI() = default;

void ParameterSelectionUI::create(lv_obj_t* parent) {
    // Create main container using the standard pattern
    UI::ContainerOptions mainOpts = {
        .parent = parent,
        .width_pct = 98,
        .height_pct = 98,
        .align = LV_ALIGN_TOP_LEFT,
        .x_offset = 0,
        .y_offset = 0,
        .bg_color = lv_color_hex(SynthConstants::Color::BG),
        .bg_opa = LV_OPA_COVER,
        .border_width = 0,
        .pad_all = 4,
        .use_bg_color = true,
        .use_bg_opa = true
    };
    main_container_ = UI::createContainer(UI::mergeOptions(UI::DefaultContainer, mainOpts));
    
    // Create compact layout for 480x320 screen
    createCompactLayout(main_container_);
    
    // Initialize with all parameters
    showAllParameters();
    
    std::cout << "[ParameterSelectionUI] Created compact 8-bit UI" << std::endl;
}

void ParameterSelectionUI::onActivated() {
    std::cout << "[ParameterSelectionUI] Activated" << std::endl;
    refreshParameterList();
}

void ParameterSelectionUI::onDeactivated() {
    std::cout << "[ParameterSelectionUI] Deactivated" << std::endl;
}

void ParameterSelectionUI::update() {
    // Update UI elements periodically
    if (selected_parameter_) {
        updateDetailsPanel();
    }
}

void ParameterSelectionUI::setSelectionCallback(ParameterSelectionCallback callback) {
    selection_callback_ = callback;
}

void ParameterSelectionUI::refreshParameterList() {
    switch (current_mode_) {
        case DisplayMode::ALL_PARAMETERS:
            showAllParameters();
            break;
        case DisplayMode::CATEGORY_FILTERED:
            showCategory(current_category_);
            break;
        case DisplayMode::SEARCH_RESULTS:
            performSearch(current_search_query_);
            break;
        case DisplayMode::FAVORITES_ONLY:
            showFavorites();
            break;
        case DisplayMode::RECENTS_ONLY:
            showRecents();
            break;
    }
}

// ============================================================================
// COMPACT LAYOUT FOR 480x320 ESP32 SCREEN
// ============================================================================

void ParameterSelectionUI::createCompactLayout(lv_obj_t* parent) {
    // Top section: Search and quick filters (height: 50px)
    createTopControls(parent);
    
    // Middle section: Split view - Parameter list (left) and details (right)
    createSplitView(parent);
    
    // Bottom section: Action buttons (height: 40px)
    createBottomActions(parent);
}

void ParameterSelectionUI::createTopControls(lv_obj_t* parent) {
    UI::ContainerOptions topOpts = {
        .parent = parent,
        .width_pct = 98,
        .height_pct = 16,  // ~50px on 320px screen
        .align = LV_ALIGN_TOP_MID,
        .x_offset = 0,
        .y_offset = 2,
        .bg_color = lv_color_hex(SynthConstants::Color::STATUS_BG),
        .bg_opa = LV_OPA_COVER,
        .border_width = 1,
        .pad_all = 2,
        .use_bg_color = true,
        .use_bg_opa = true
    };
    search_container_ = UI::createContainer(UI::mergeOptions(UI::DefaultContainer, topOpts));
    lv_obj_set_style_border_color(search_container_, lv_color_hex(SynthConstants::Color::STATUS_BORDER), 0);
    
    // Create horizontal layout for controls
    lv_obj_set_layout(search_container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(search_container_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(search_container_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    
    // Synth label (small)
    lv_obj_t* synth_label = lv_label_create(search_container_);
    lv_label_set_text(synth_label, "SYNTH:");
    lv_obj_set_style_text_font(synth_label, FontA.small, 0);  // PressStart2P_6
    lv_obj_set_style_text_color(synth_label, lv_color_hex(SynthConstants::Color::HELP), 0);
    
    // Search input (compact)
    search_input_ = lv_textarea_create(search_container_);
    lv_obj_set_size(search_input_, 120, 30);
    lv_textarea_set_one_line(search_input_, true);
    lv_textarea_set_placeholder_text(search_input_, "Search...");
    lv_obj_set_style_text_font(search_input_, FontA.small, 0);
    lv_obj_set_style_bg_color(search_input_, lv_color_hex(0x222222), 0);
    lv_obj_set_style_text_color(search_input_, lv_color_hex(0x00FF00), 0);
    lv_obj_add_event_cb(search_input_, onSearchInputChanged, LV_EVENT_VALUE_CHANGED, this);
    
    // Quick filter buttons (compact)
    createQuickFilterButton(search_container_, "FAV", &favorites_btn_, onFavoritesButtonClicked);
    createQuickFilterButton(search_container_, "REC", &recents_btn_, onRecentsButtonClicked);
    
    // Category dropdown (small)
    synth_dropdown_ = lv_dropdown_create(search_container_);
    lv_obj_set_size(synth_dropdown_, 80, 30);
    lv_obj_set_style_text_font(synth_dropdown_, FontA.small, 0);
    lv_dropdown_set_options(synth_dropdown_, "ALL\\nOSC\\nFLT\\nENV\\nLFO\\nFX\\nMCR");
    lv_obj_add_event_cb(synth_dropdown_, onCategoryDropdownChanged, LV_EVENT_VALUE_CHANGED, this);
}

void ParameterSelectionUI::createSplitView(lv_obj_t* parent) {
    // Middle container for split view
    UI::ContainerOptions midOpts = {
        .parent = parent,
        .width_pct = 98,
        .height_pct = 72,  // ~230px on 320px screen
        .align = LV_ALIGN_TOP_LEFT,
        .x_offset = 0,
        .y_offset = 0,
        .bg_color = lv_color_hex(SynthConstants::Color::BG),
        .bg_opa = LV_OPA_TRANSP,
        .border_width = 0,
        .pad_all = 2,
        .use_bg_color = false,
        .use_bg_opa = true
    };
    lv_obj_t* split_container = UI::createContainer(UI::mergeOptions(UI::DefaultContainer, midOpts));
    lv_obj_align_to(split_container, search_container_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);
    
    // Set horizontal layout for split view
    lv_obj_set_layout(split_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(split_container, LV_FLEX_FLOW_ROW);
    
    // Left side: Parameter list (60% width)
    createParameterList(split_container);
    
    // Right side: Details panel (40% width)
    createDetailsPanel(split_container);
}

void ParameterSelectionUI::createBottomActions(lv_obj_t* parent) {
    UI::ContainerOptions bottomOpts = {
        .parent = parent,
        .width_pct = 98,
        .height_pct = 12,  // ~40px on 320px screen
        .align = LV_ALIGN_BOTTOM_MID,
        .x_offset = 0,
        .y_offset = -2,
        .bg_color = lv_color_hex(SynthConstants::Color::BTN_UNDO_BG),
        .bg_opa = LV_OPA_COVER,
        .border_width = 1,
        .pad_all = 2,
        .use_bg_color = true,
        .use_bg_opa = true
    };
    action_buttons_ = UI::createContainer(UI::mergeOptions(UI::DefaultContainer, bottomOpts));
    lv_obj_set_style_border_color(action_buttons_, lv_color_hex(SynthConstants::Color::BTN_UNDO_BORDER), 0);
    
    // Horizontal layout for action buttons
    lv_obj_set_layout(action_buttons_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(action_buttons_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(action_buttons_, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Create action buttons with 8-bit styling
    createActionButton(action_buttons_, "ASSIGN", onAssignButtonClicked, SynthConstants::Color::BTN_FILTER_ON);
    createActionButton(action_buttons_, "STAR", onFavoriteButtonClicked, SynthConstants::Color::BTN_LFO_ON);
    createActionButton(action_buttons_, "MACRO", onMacroButtonClicked, SynthConstants::Color::BTN_TRIGGER_ON);
}

void ParameterSelectionUI::createParameterList(lv_obj_t* parent) {
    UI::ContainerOptions listOpts = {
        .parent = parent,
        .width_pct = 58,
        .height_pct = 100,
        .align = LV_ALIGN_TOP_LEFT,
        .x_offset = 0,
        .y_offset = 0,
        .bg_color = lv_color_hex(0x111111),
        .bg_opa = LV_OPA_COVER,
        .border_width = 1,
        .pad_all = 3,
        .use_bg_color = true,
        .use_bg_opa = true
    };
    parameter_list_ = UI::createContainer(UI::mergeOptions(UI::DefaultContainer, listOpts));
    lv_obj_set_style_border_color(parameter_list_, lv_color_hex(0x444444), 0);
    
    // Enable scrolling
    lv_obj_set_scrollbar_mode(parameter_list_, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(parameter_list_, LV_DIR_VER);
    
    // Vertical flex layout for parameter items
    lv_obj_set_layout(parameter_list_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parameter_list_, LV_FLEX_FLOW_COLUMN);
}

void ParameterSelectionUI::createDetailsPanel(lv_obj_t* parent) {
    UI::ContainerOptions detailOpts = {
        .parent = parent,
        .width_pct = 40,
        .height_pct = 100,
        .align = LV_ALIGN_TOP_RIGHT,
        .x_offset = 0,
        .y_offset = 0,
        .bg_color = lv_color_hex(0x0a0a0a),
        .bg_opa = LV_OPA_COVER,
        .border_width = 1,
        .pad_all = 4,
        .use_bg_color = true,
        .use_bg_opa = true
    };
    details_panel_ = UI::createContainer(UI::mergeOptions(UI::DefaultContainer, detailOpts));
    lv_obj_set_style_border_color(details_panel_, lv_color_hex(0x444444), 0);
    
    // Initially show help text
    lv_obj_t* help_label = lv_label_create(details_panel_);
    lv_label_set_text(help_label, "SELECT\nPARAM\nFOR INFO");
    lv_obj_set_style_text_font(help_label, FontA.small, 0);
    lv_obj_set_style_text_color(help_label, lv_color_hex(SynthConstants::Color::HELP), 0);
    lv_obj_set_style_text_align(help_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(help_label);
}

// ============================================================================
// HELPER FUNCTIONS FOR 8-BIT STYLED COMPONENTS
// ============================================================================

void ParameterSelectionUI::createQuickFilterButton(lv_obj_t* parent, const char* text, lv_obj_t** btn_ptr, lv_event_cb_t callback) {
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 35, 25);
    lv_obj_set_style_bg_color(btn, lv_color_hex(SynthConstants::Color::BTN_FILTER_OFF), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(SynthConstants::Color::BTN_FILTER_ON), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x666666), 0);
    lv_obj_set_style_radius(btn, 2, 0);
    
    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, FontA.small, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(label);
    
    lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, this);
    *btn_ptr = btn;
}

void ParameterSelectionUI::createActionButton(lv_obj_t* parent, const char* text, lv_event_cb_t callback, uint32_t color) {
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 65, 28);
    lv_obj_set_style_bg_color(btn, lv_color_hex(SynthConstants::Color::BTN_UNDO_BG), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(color), 0);
    lv_obj_set_style_radius(btn, 2, 0);
    
    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, FontA.small, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_center(label);
    
    lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, this);
}

// ============================================================================
// PARAMETER DISPLAY METHODS - COMPACT 8-BIT STYLE
// ============================================================================

void ParameterSelectionUI::populateParameterList(const std::vector<std::shared_ptr<Parameter>>& parameters) {
    clearParameterList();
    
    std::cout << "[ParameterSelectionUI] Displaying " << parameters.size() << " parameters" << std::endl;
    
    for (auto parameter : parameters) {
        lv_obj_t* item = createCompactParameterItem(parameter);
        parameter_items_.push_back(item);
    }
}

void ParameterSelectionUI::clearParameterList() {
    for (auto item : parameter_items_) {
        lv_obj_del(item);
    }
    parameter_items_.clear();
}

lv_obj_t* ParameterSelectionUI::createCompactParameterItem(std::shared_ptr<Parameter> parameter) {
    lv_obj_t* item = lv_obj_create(parameter_list_);
    lv_obj_set_size(item, LV_PCT(96), 22);  // Compact height for more items
    lv_obj_set_style_bg_color(item, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_bg_color(item, lv_color_hex(SynthConstants::Color::BTN_FILTER_ON), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(item, 1, 0);
    lv_obj_set_style_border_color(item, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(item, 1, 0);
    lv_obj_set_style_pad_all(item, 2, 0);
    
    // Store parameter pointer in user data
    lv_obj_set_user_data(item, parameter.get());
    
    // Create horizontal layout
    lv_obj_set_layout(item, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    
    // Category color indicator (4px wide)
    lv_obj_t* color_indicator = lv_obj_create(item);
    lv_obj_set_size(color_indicator, 3, 16);
    lv_obj_set_style_bg_color(color_indicator, getCategoryColor(parameter->getCategory()), 0);
    lv_obj_set_style_border_width(color_indicator, 0, 0);
    lv_obj_set_style_radius(color_indicator, 0, 0);
    
    // Parameter name (truncated if needed)
    lv_obj_t* name_label = lv_label_create(item);
    std::string display_name = parameter->getShortName();
    if (display_name.length() > 12) {
        display_name = display_name.substr(0, 12);
    }
    lv_label_set_text(name_label, display_name.c_str());
    lv_obj_set_style_text_font(name_label, FontA.small, 0);
    lv_obj_set_style_text_color(name_label, lv_color_hex(0xFFFFFF), 0);
    
    // CC number (right aligned)
    lv_obj_t* cc_label = lv_label_create(item);
    std::string cc_text = "CC" + std::to_string(parameter->getCCNumber());
    lv_label_set_text(cc_label, cc_text.c_str());
    lv_obj_set_style_text_font(cc_label, FontA.small, 0);
    lv_obj_set_style_text_color(cc_label, lv_color_hex(SynthConstants::Color::STATUS), 0);
    
    // Status indicators (favorites, assigned)
    if (synth_param_manager_->isFavorite(parameter->getName())) {
        lv_obj_t* fav_icon = lv_label_create(item);
        lv_label_set_text(fav_icon, "*");
        lv_obj_set_style_text_font(fav_icon, FontA.small, 0);
        lv_obj_set_style_text_color(fav_icon, lv_color_hex(SynthConstants::Color::BTN_LFO_ON), 0);
    }
    
    // Make item clickable
    lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(item, onParameterItemClicked, LV_EVENT_CLICKED, this);
    
    updateParameterItemAppearance(item, parameter);
    
    return item;
}

void ParameterSelectionUI::updateParameterItemAppearance(lv_obj_t* item, std::shared_ptr<Parameter> parameter) {
    // Update appearance based on selection state
    if (selected_parameter_ && selected_parameter_->getName() == parameter->getName()) {
        lv_obj_set_style_bg_color(item, lv_color_hex(SynthConstants::Color::BTN_FILTER_ON), 0);
        lv_obj_set_style_bg_opa(item, 100, 0);
    } else {
        lv_obj_set_style_bg_color(item, lv_color_hex(0x1a1a1a), 0);
        lv_obj_set_style_bg_opa(item, 255, 0);
    }
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

void ParameterSelectionUI::onSearchInputChanged(lv_event_t* e) {
    ParameterSelectionUI* self = static_cast<ParameterSelectionUI*>(lv_event_get_user_data(e));
    const char* search_text = lv_textarea_get_text(self->search_input_);
    self->performSearch(std::string(search_text));
}

void ParameterSelectionUI::onCategoryDropdownChanged(lv_event_t* e) {
    ParameterSelectionUI* self = static_cast<ParameterSelectionUI*>(lv_event_get_user_data(e));
    uint16_t selected = lv_dropdown_get_selected(self->synth_dropdown_);
    
    // Map dropdown index to category
    ParameterCategory category = ParameterCategory::UNKNOWN;
    switch (selected) {
        case 0: category = ParameterCategory::UNKNOWN; break;
        case 1: category = ParameterCategory::OSCILLATORS; break;
        case 2: category = ParameterCategory::FILTERS; break;
        case 3: category = ParameterCategory::ENVELOPES; break;
        case 4: category = ParameterCategory::LFOS; break;
        case 5: category = ParameterCategory::EFFECTS; break;
        case 6: category = ParameterCategory::MACROS; break;
    }
    
    self->showCategory(category);
}

void ParameterSelectionUI::onParameterItemClicked(lv_event_t* e) {
    ParameterSelectionUI* self = static_cast<ParameterSelectionUI*>(lv_event_get_user_data(e));
    lv_obj_t* item = static_cast<lv_obj_t*>(lv_event_get_target(e));
    
    Parameter* param_ptr = static_cast<Parameter*>(lv_obj_get_user_data(item));
    if (param_ptr) {
        // Find the shared_ptr for this parameter
        auto parameters = self->synth_param_manager_->getParametersByCategory(ParameterCategory::UNKNOWN);
        for (auto param : parameters) {
            if (param.get() == param_ptr) {
                self->selectParameter(param);
                break;
            }
        }
    }
}

void ParameterSelectionUI::onFavoritesButtonClicked(lv_event_t* e) {
    ParameterSelectionUI* self = static_cast<ParameterSelectionUI*>(lv_event_get_user_data(e));
    self->showFavorites();
}

void ParameterSelectionUI::onRecentsButtonClicked(lv_event_t* e) {
    ParameterSelectionUI* self = static_cast<ParameterSelectionUI*>(lv_event_get_user_data(e));
    self->showRecents();
}

void ParameterSelectionUI::onAssignButtonClicked(lv_event_t* e) {
    ParameterSelectionUI* self = static_cast<ParameterSelectionUI*>(lv_event_get_user_data(e));
    self->handleParameterAssign();
}

void ParameterSelectionUI::onFavoriteButtonClicked(lv_event_t* e) {
    ParameterSelectionUI* self = static_cast<ParameterSelectionUI*>(lv_event_get_user_data(e));
    self->handleParameterFavorite();
}

void ParameterSelectionUI::onMacroButtonClicked(lv_event_t* e) {
    ParameterSelectionUI* self = static_cast<ParameterSelectionUI*>(lv_event_get_user_data(e));
    self->handleParameterMacro();
}

// ============================================================================
// DISPLAY MODE METHODS
// ============================================================================

void ParameterSelectionUI::performSearch(const std::string& query) {
    current_mode_ = DisplayMode::SEARCH_RESULTS;
    current_search_query_ = query;
    
    auto results = synth_param_manager_->searchParameters(query, current_category_);
    populateParameterList(results);
}

void ParameterSelectionUI::showFavorites() {
    current_mode_ = DisplayMode::FAVORITES_ONLY;
    setButtonStyle(favorites_btn_, true);
    setButtonStyle(recents_btn_, false);
    
    auto favorites = synth_param_manager_->getFavoriteParameters();
    populateParameterList(favorites);
}

void ParameterSelectionUI::showRecents() {
    current_mode_ = DisplayMode::RECENTS_ONLY;
    setButtonStyle(recents_btn_, true);
    setButtonStyle(favorites_btn_, false);
    
    auto recents = synth_param_manager_->getRecentParameters();
    populateParameterList(recents);
}

void ParameterSelectionUI::showAllParameters() {
    current_mode_ = DisplayMode::ALL_PARAMETERS;
    setButtonStyle(favorites_btn_, false);
    setButtonStyle(recents_btn_, false);
    
    auto parameters = synth_param_manager_->getParametersByCategory(ParameterCategory::UNKNOWN);
    populateParameterList(parameters);
}

void ParameterSelectionUI::showCategory(ParameterCategory category) {
    current_mode_ = DisplayMode::CATEGORY_FILTERED;
    current_category_ = category;
    
    auto parameters = synth_param_manager_->getParametersByCategory(category);
    populateParameterList(parameters);
}

// ============================================================================
// HELPER METHODS
// ============================================================================

void ParameterSelectionUI::selectParameter(std::shared_ptr<Parameter> parameter) {
    selected_parameter_ = parameter;
    
    // Update visual selection in list
    for (auto item : parameter_items_) {
        Parameter* item_param = static_cast<Parameter*>(lv_obj_get_user_data(item));
        if (item_param == parameter.get()) {
            updateParameterItemAppearance(item, parameter);
        } else {
            // Find corresponding parameter for this item
            auto all_params = synth_param_manager_->getParametersByCategory(ParameterCategory::UNKNOWN);
            for (auto p : all_params) {
                if (p.get() == item_param) {
                    updateParameterItemAppearance(item, p);
                    break;
                }
            }
        }
    }
    
    updateDetailsPanel();
    
    std::cout << "[ParameterSelectionUI] Selected: " << parameter->getName() << std::endl;
}

void ParameterSelectionUI::updateDetailsPanel() {
    if (!selected_parameter_) return;
    
    // Clear existing content
    lv_obj_clean(details_panel_);
    
    // Parameter name
    lv_obj_t* name_label = lv_label_create(details_panel_);
    std::string name = selected_parameter_->getName();
    if (name.length() > 16) {
        name = name.substr(0, 16) + "...";
    }
    lv_label_set_text(name_label, name.c_str());
    lv_obj_set_style_text_font(name_label, FontA.med, 0);  // PressStart2P_8
    lv_obj_set_style_text_color(name_label, lv_color_hex(SynthConstants::Color::TITLE), 0);
    lv_obj_align(name_label, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // CC and range info
    lv_obj_t* info_label = lv_label_create(details_panel_);
    std::string info = "CC" + std::to_string(selected_parameter_->getCCNumber()) + "\n" +
                      std::to_string(selected_parameter_->getMinValue()) + "-" + 
                      std::to_string(selected_parameter_->getMaxValue()) + "\n" +
                      "DEF:" + std::to_string(selected_parameter_->getDefaultValue());
    lv_label_set_text(info_label, info.c_str());
    lv_obj_set_style_text_font(info_label, FontA.small, 0);
    lv_obj_set_style_text_color(info_label, lv_color_hex(SynthConstants::Color::STATUS), 0);
    lv_obj_align_to(info_label, name_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);
    
    // Status indicators
    lv_obj_t* status_label = lv_label_create(details_panel_);
    std::string status = "";
    if (synth_param_manager_->isFavorite(selected_parameter_->getName())) {
        status += "FAV ";
    }
    if (synth_param_manager_->isParameterAssigned(selected_parameter_->getName())) {
        status += "ASSGN";
    }
    if (status.empty()) {
        status = "READY";
    }
    lv_label_set_text(status_label, status.c_str());
    lv_obj_set_style_text_font(status_label, FontA.small, 0);
    lv_obj_set_style_text_color(status_label, lv_color_hex(SynthConstants::Color::BTN_LFO_ON), 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

lv_color_t ParameterSelectionUI::getCategoryColor(ParameterCategory category) const {
    switch (category) {
        case ParameterCategory::SYSTEM: return lv_color_hex(SynthConstants::Color::DIAL_PINK);
        case ParameterCategory::VOICE: return lv_color_hex(SynthConstants::Color::DIAL_GREEN);
        case ParameterCategory::OSCILLATORS: return lv_color_hex(SynthConstants::Color::DIAL_BLUE);
        case ParameterCategory::MIXER: return lv_color_hex(SynthConstants::Color::DIAL_YELLOW_GREEN);
        case ParameterCategory::FILTERS: return lv_color_hex(SynthConstants::Color::DIAL_ORANGE);
        case ParameterCategory::ENVELOPES: return lv_color_hex(SynthConstants::Color::DIAL_MAGENTA);
        case ParameterCategory::LFOS: return lv_color_hex(SynthConstants::Color::DIAL_GREEN);
        case ParameterCategory::MUTATORS: return lv_color_hex(SynthConstants::Color::DIAL_PINK);
        case ParameterCategory::MACROS: return lv_color_hex(SynthConstants::Color::STATUS);
        case ParameterCategory::ARPEGGIATOR: return lv_color_hex(SynthConstants::Color::DIAL_BLUE);
        case ParameterCategory::EFFECTS: return lv_color_hex(SynthConstants::Color::DIAL_ORANGE);
        case ParameterCategory::AMPLITUDE: return lv_color_hex(SynthConstants::Color::DIAL_MAGENTA);
        default: return lv_color_hex(0x666666);
    }
}

void ParameterSelectionUI::handleParameterAssign() {
    if (!selected_parameter_) return;
    
    if (selection_callback_) {
        selection_callback_(selected_parameter_, "assign");
    }
}

void ParameterSelectionUI::handleParameterFavorite() {
    if (!selected_parameter_) return;
    
    bool is_favorite = synth_param_manager_->isFavorite(selected_parameter_->getName());
    
    if (is_favorite) {
        synth_param_manager_->removeFromFavorites(selected_parameter_->getName());
    } else {
        synth_param_manager_->addToFavorites(selected_parameter_->getName());
    }
    
    // Refresh display
    refreshParameterList();
    updateDetailsPanel();
}

void ParameterSelectionUI::handleParameterMacro() {
    if (!selected_parameter_) return;
    
    if (selection_callback_) {
        selection_callback_(selected_parameter_, "macro");
    }
}

void ParameterSelectionUI::setButtonStyle(lv_obj_t* btn, bool active) {
    if (active) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(SynthConstants::Color::BTN_FILTER_ON), 0);
    } else {
        lv_obj_set_style_bg_color(btn, lv_color_hex(SynthConstants::Color::BTN_FILTER_OFF), 0);
    }
}