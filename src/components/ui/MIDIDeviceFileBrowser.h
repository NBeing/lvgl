/**
 * @brief Example integration of LVGLFileBrowser with MIDI Framework
 * 
 * Shows how to use the cross-platform file browser in your ESP32/Desktop
 * MIDI device application.
 */

#include "LVGLFileBrowser.h"
#include <memory>

class MIDIDeviceFileBrowser {
private:
    std::unique_ptr<LVGLFileBrowser> browser_;
    lv_obj_t* info_label_;
    
public:
    /**
     * @brief Create MIDI file browser with storage info
     */
    MIDIDeviceFileBrowser(lv_obj_t* parent) {
        #ifdef ESP32_BUILD
            // Initialize SD card with custom pins for your board
            // Adjust these pins based on your ESP32-S3 board layout
            LVGLFileBrowser::initializeSDCard(
                10,  // CS pin (adjust for your board)
                12,  // SCK pin  
                11,  // MOSI pin
                13   // MISO pin
            );
        #endif
        
        // Create main container
        lv_obj_t* container = lv_obj_create(parent);
        lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
        lv_obj_set_layout(container, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
        
        // Storage info label
        info_label_ = lv_label_create(container);
        lv_obj_set_width(info_label_, LV_PCT(100));
        lv_label_set_text(info_label_, "Storage: Checking...");
        
        // File browser
        browser_ = std::make_unique<LVGLFileBrowser>(container);
        
        // Set MIDI-specific file filters
        browser_->setFileFilters({".mid", ".midi", ".json", ".cfg", ".txt"});
        
        // Set up callbacks
        browser_->setFileSelectedCallback([this](const FileEntry& file) {
            handleFileSelected(file);
        });
        
        browser_->setDirectoryChangedCallback([this](const std::string& path) {
            updateStorageInfo(path);
        });
        
        // Initial storage info update
        updateStorageInfo(browser_->getCurrentPath());
    }
    
private:
    void handleFileSelected(const FileEntry& file) {
        lv_label_set_text_fmt(info_label_, "Selected: %s (%zu bytes)", 
                              file.name.c_str(), file.size);
        
        if (file.extension == "mid" || file.extension == "midi") {
            loadMIDIFile(file.full_path);
        } else if (file.extension == "json") {
            loadSynthConfig(file.full_path);
        } else if (file.extension == "cfg") {
            loadDeviceConfig(file.full_path);
        }
    }
    
    void updateStorageInfo(const std::string& path) {
        #ifdef ESP32_BUILD
            if (path.substr(0, 7) == "/spiffs") {
                size_t total = SPIFFS.totalBytes();
                size_t used = SPIFFS.usedBytes();
                size_t free = total - used;
                lv_label_set_text_fmt(info_label_, 
                    "SPIFFS: %zu KB free / %zu KB total", 
                    free / 1024, total / 1024);
            } else if (path.substr(0, 3) == "/sd") {
                size_t total = SD.totalBytes();
                size_t used = SD.usedBytes(); 
                size_t free = total - used;
                lv_label_set_text_fmt(info_label_,
                    "SD Card: %zu MB free / %zu MB total",
                    free / (1024 * 1024), total / (1024 * 1024));
            } else {
                lv_label_set_text(info_label_, "Storage: Select filesystem");
            }
        #else
            lv_label_set_text_fmt(info_label_, "Desktop: %s", path.c_str());
        #endif
    }
    
    void loadMIDIFile(const std::string& path) {
        // Your MIDI file loading logic here
        LV_LOG_INFO("Loading MIDI file: %s", path.c_str());
        
        // Example: Read file and parse MIDI data
        // This is where you'd integrate with your MIDI player/sequencer
    }
    
    void loadSynthConfig(const std::string& path) {
        // Your synthesizer configuration loading
        LV_LOG_INFO("Loading synth config: %s", path.c_str());
        
        // Example: Parse JSON configuration file
        // Update your synthesizer parameters
    }
    
    void loadDeviceConfig(const std::string& path) {
        // Your device configuration loading
        LV_LOG_INFO("Loading device config: %s", path.c_str());
        
        // Example: Load device settings, MIDI mappings, etc.
    }
};

// Usage example in your main application:
/*
void setupFileBrowser() {
    // Create file browser tab/window
    lv_obj_t* tab = lv_tabview_add_tab(tabview, "Files");
    
    // Create the MIDI file browser
    auto file_browser = std::make_unique<MIDIDeviceFileBrowser>(tab);
    
    // The browser is now ready to use!
    // - ESP32: Shows /spiffs and /sd filesystems
    // - Desktop: Shows regular filesystem
    // - Touch-friendly interface
    // - MIDI file filtering
    // - Storage space monitoring
}
*/
