#pragma once

namespace SynthConstants {
    // Screen
    inline constexpr int ESP32_SCREEN_WIDTH = 480;
    inline constexpr int ESP32_SCREEN_HEIGHT = 320;

    // --- Color palette ---
    namespace Color {
        inline constexpr int BG = 0x000000; // Black background
        inline constexpr int TITLE = 0x00FF00;
        inline constexpr int HELP = 0xCCCCCC;
        inline constexpr int STATUS = 0xFFFF00;
        inline constexpr int DIAL_GREEN = 0x00FF00;
        inline constexpr int DIAL_ORANGE = 0xFF8000;
        inline constexpr int DIAL_BLUE = 0x0080FF;
        inline constexpr int DIAL_MAGENTA = 0xFF00FF;
        inline constexpr int DIAL_YELLOW_GREEN = 0x80FF00;
        inline constexpr int DIAL_PINK = 0xFF0080;
        inline constexpr int BTN_FILTER_OFF = 0x444444;
        inline constexpr int BTN_FILTER_ON = 0x00AA00;
        inline constexpr int BTN_LFO_OFF = 0x444444;
        inline constexpr int BTN_LFO_ON = 0xFF8800;
        inline constexpr int BTN_TRIGGER_OFF = 0x666666;
        inline constexpr int BTN_TRIGGER_ON = 0xFF0000;
        inline constexpr int BTN_UNDO_BG = 0x333333;
        inline constexpr int BTN_UNDO_BORDER = 0x888888;
        inline constexpr int BTN_UNDO_ACTIVE = 0x004400;
        inline constexpr int BTN_UNDO_TEXT = 0x00FF00;
        inline constexpr int BTN_UNDO_TEXT_DISABLED = 0x666666;
        inline constexpr int BTN_REDO_BG = 0x333333;
        inline constexpr int BTN_REDO_BORDER = 0x888888;
        inline constexpr int BTN_REDO_ACTIVE = 0x444400;
        inline constexpr int BTN_REDO_TEXT = 0xFFFF00;
        inline constexpr int BTN_REDO_TEXT_DISABLED = 0x666666;
        inline constexpr int SIM_BTN_BG = 0x333333;
        inline constexpr int SIM_BTN_BORDER = 0x00FF00;
        inline constexpr int SIM_BTN_TEXT = 0x00FF00;
        inline constexpr int STATUS_BG = 0x1a1a1a;
        inline constexpr int STATUS_BORDER = 0x444444;
    }

    // --- Text/labels ---
    namespace Text {
        inline constexpr const char* TITLE = "LVGL Synth GUI - Parameter System";
        inline constexpr const char* HELP = "NEW: Parameter-aware dials (top) • OLD: Legacy dials (bottom)";
        inline constexpr const char* BTN_FILTER = "FILTER";
        inline constexpr const char* BTN_LFO = "LFO SYNC";
        inline constexpr const char* BTN_TRIGGER = "TRIGGER";
        inline constexpr const char* BTN_UNDO = "UNDO";
        inline constexpr const char* BTN_REDO = "REDO";
        inline constexpr const char* BTN_SIMULATE = "Simulate Values";
        inline constexpr const char* STATUS_READY = "Parameter System Ready";
        inline constexpr const char* STATUS_READY_LG = "Ready - Compare NEW vs OLD dial systems";
        inline constexpr const char* STATUS_SIM = "SIMULATION";
        inline constexpr const char* STATUS_FORMAT = "Last: %s = %d (CC%d)";
        inline constexpr const char* STATUS_UNDO_REDO_FORMAT = "Undo: %s | Redo: %s";
    }

    // --- MIDI/Parameter ---
    namespace Midi {
        inline constexpr int CHANNEL = 1;
        inline constexpr int SIM_VALUES[5] = {0, 32, 64, 96, 127};
        inline constexpr int BTN_TOGGLE_OFF = 0;
        inline constexpr int BTN_TOGGLE_ON = 127;
        inline constexpr int BTN_TRIGGER_ON = 127;
        inline constexpr int BTN_TRIGGER_OFF = 0;
    }

    // --- UI Layout ---
    namespace Layout {
        // UI scaling factor for ESP32 vs Desktop
        #if defined(ESP32_BUILD)
          inline constexpr float UI_SCALE = 1.0f;
        #else
          inline constexpr float UI_SCALE = 1.0f;
        #endif

        // Set this to 0 (SMALL), 1 (MEDIUM), 2 (LARGE) to force a screen size, or -1 to auto-detect
        inline constexpr int USER_SCREEN_SIZE_OVERRIDE = 0; // -1 = auto, 0 = SMALL, 1 = MEDIUM, 2 = LARGE
        
        inline constexpr int TITLE_Y_SMALL = 60;
        inline constexpr int TITLE_Y_LARGE = 70;
        inline constexpr int HELP_LABEL_Y_OFFSET = 25;
        inline constexpr int BUTTON_BELOW_DIALS_SPACING = 12;
        inline constexpr int BUTTON_X_OFFSET_LG = 20;
        inline constexpr int BUTTON_Y_LG = 80;
        inline constexpr int UNDO_X_LG = 100;
        inline constexpr int UNDO_Y_LG = 20;
        inline constexpr int SIM_BTN_WIDTH = 200;
        inline constexpr int SIM_BTN_Y_OFFSET = 10;
        inline constexpr int STATUS_BORDER_WIDTH = 1;
        inline constexpr int STATUS_RADIUS = 4;
        inline constexpr int STATUS_BG_OPA = 0xFF;

        // --- LEGACY/ABSOLUTE LAYOUT CONSTANTS TO REMOVE (for refactor) ---
        // These are kept for compatibility but should be removed as flex layouts are adopted everywhere.
        // Search for usages and refactor to use container-relative/flex-based layout only.
        // (Do not add new code using these!)
        inline constexpr int DEFAULT_DIAL_SIZE = 80;
        inline constexpr int DEFAULT_DIAL_SPACING_X = 120;
        inline constexpr int DEFAULT_DIAL_SPACING_Y = 120;
        inline constexpr int DEFAULT_MARGIN_X = 50;
        inline constexpr int DEFAULT_MARGIN_Y = 50;
        inline constexpr int DEFAULT_BUTTON_WIDTH = 100;
        inline constexpr int DEFAULT_BUTTON_HEIGHT = 40;
        inline constexpr int DEFAULT_BUTTON_SPACING = 20;
        inline constexpr int DEFAULT_STATUS_HEIGHT = 32;
        inline constexpr int DEFAULT_GRID_COLS = 3;
        inline constexpr int DEFAULT_GRID_ROWS = 2;
    }

    // --- Buffer sizes ---
    namespace Buffer {
        inline constexpr int STATUS = 100;
        inline constexpr int STATUS_UNDO_REDO = 150;
    }
}
