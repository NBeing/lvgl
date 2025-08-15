/**
 * @brief Cross-Platform File Browser for LVGL MIDI Framework
 * 
 * A lightweight, cross-platform file browser designed specifically for
 * MIDI device applications, supporting both ESP32 (SPIFFS/SD) and 
 * desktop environments.
 */

#pragma once
#include <lvgl.h>
#include <vector>
#include <string>
#include <functional>

#ifdef ESP32_BUILD
    #include <SPIFFS.h>
    #include <SD.h>
    #include <FS.h>
#else
    #include <filesystem>
    namespace fs = std::filesystem;
#endif

/**
 * @brief File system entry information
 */
struct FileEntry {
    std::string name;
    std::string full_path;
    bool is_directory;
    size_t size;
    std::string extension;
    
    FileEntry(const std::string& n, const std::string& path, bool dir, size_t s = 0)
        : name(n), full_path(path), is_directory(dir), size(s) {
        if (!dir && name.find('.') != std::string::npos) {
            extension = name.substr(name.find_last_of('.') + 1);
        }
    }
};

/**
 * @brief Cross-platform file browser widget for LVGL
 * 
 * Features:
 * - Cross-platform file system access
 * - MIDI file filtering (.mid, .json, .txt)
 * - Touch-friendly interface
 * - Configurable file type filters
 * - Navigation breadcrumbs
 * - File selection callbacks
 */
class LVGLFileBrowser {
public:
    using FileSelectedCallback = std::function<void(const FileEntry&)>;
    using DirectoryChangedCallback = std::function<void(const std::string&)>;
    
private:
    lv_obj_t* container_;
    lv_obj_t* breadcrumb_label_;
    lv_obj_t* file_list_;
    lv_obj_t* up_button_;
    
    std::string current_path_;
    std::vector<FileEntry> current_entries_;
    std::vector<std::string> file_filters_;
    
    FileSelectedCallback file_selected_cb_;
    DirectoryChangedCallback dir_changed_cb_;
    
public:
    /**
     * @brief Create a new file browser widget
     */
    LVGLFileBrowser(lv_obj_t* parent) {
        createUI(parent);
        
        // Default MIDI-related file filters
        file_filters_ = {".mid", ".midi", ".json", ".txt", ".cfg"};
        
        // Start at root directory
        #ifdef ESP32_BUILD
            // Try to mount both SPIFFS and SD card
            if (!SPIFFS.begin(true)) {
                LV_LOG_WARN("SPIFFS Mount Failed");
            }
            
            // Try to initialize SD card (adjust pin for your board)
            if (!SD.begin()) {
                LV_LOG_INFO("SD Card not found, using SPIFFS only");
                current_path_ = "/spiffs";
            } else {
                LV_LOG_INFO("SD Card mounted successfully");
                current_path_ = "/sd";  // Start with SD card if available
            }
        #else
            // Desktop: Start in current directory
            current_path_ = fs::current_path().string();
        #endif
        
        refreshDirectory();
    }
    
    /**
     * @brief Set file selection callback
     */
    void setFileSelectedCallback(FileSelectedCallback callback) {
        file_selected_cb_ = callback;
    }
    
    /**
     * @brief Set directory changed callback
     */
    void setDirectoryChangedCallback(DirectoryChangedCallback callback) {
        dir_changed_cb_ = callback;
    }
    
    /**
     * @brief Set file type filters
     */
    void setFileFilters(const std::vector<std::string>& filters) {
        file_filters_ = filters;
        refreshDirectory();
    }
    
    /**
     * @brief Navigate to specific directory
     */
    void navigateToDirectory(const std::string& path) {
        current_path_ = path;
        refreshDirectory();
    }
    
    /**
     * @brief Get LVGL container object
     */
    lv_obj_t* getContainer() const {
        return container_;
    }

    #ifdef ESP32_BUILD
    /**
     * @brief Initialize SD card with custom pins (call before creating browser)
     * @param cs_pin Chip select pin (default: 5)
     * @param sck_pin SPI clock pin (default: 18) 
     * @param mosi_pin SPI MOSI pin (default: 23)
     * @param miso_pin SPI MISO pin (default: 19)
     */
    static bool initializeSDCard(int cs_pin = 5, int sck_pin = 18, int mosi_pin = 23, int miso_pin = 19) {
        SPIClass spi(VSPI);
        spi.begin(sck_pin, miso_pin, mosi_pin, cs_pin);
        
        if (!SD.begin(cs_pin, spi)) {
            LV_LOG_WARN("SD Card initialization failed");
            return false;
        }
        
        LV_LOG_INFO("SD Card initialized successfully");
        return true;
    }
    
    /**
     * @brief Get available storage space for current filesystem
     */
    size_t getAvailableSpace() const {
        #ifdef ESP32_BUILD
            if (current_path_.substr(0, 7) == "/spiffs") {
                return SPIFFS.totalBytes() - SPIFFS.usedBytes();
            } else if (current_path_.substr(0, 3) == "/sd") {
                return SD.totalBytes() - SD.usedBytes();
            }
        #endif
        return 0;
    }
    #endif

private:
    /**
     * @brief Create the UI layout
     */
    void createUI(lv_obj_t* parent) {
        // Main container
        container_ = lv_obj_create(parent);
        lv_obj_set_size(container_, LV_PCT(100), LV_PCT(100));
        lv_obj_set_layout(container_, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
        
        // Navigation bar
        lv_obj_t* nav_bar = lv_obj_create(container_);
        lv_obj_set_size(nav_bar, LV_PCT(100), 40);
        lv_obj_set_layout(nav_bar, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(nav_bar, LV_FLEX_FLOW_ROW);
        
        // Up button
        up_button_ = lv_btn_create(nav_bar);
        lv_obj_set_size(up_button_, 40, LV_PCT(100));
        lv_obj_add_event_cb(up_button_, upButtonCallback, LV_EVENT_CLICKED, this);
        
        lv_obj_t* up_label = lv_label_create(up_button_);
        lv_label_set_text(up_label, LV_SYMBOL_UP);
        lv_obj_center(up_label);
        
        // Breadcrumb
        breadcrumb_label_ = lv_label_create(nav_bar);
        lv_obj_set_flex_grow(breadcrumb_label_, 1);
        lv_label_set_long_mode(breadcrumb_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
        
        // File list
        file_list_ = lv_list_create(container_);
        lv_obj_set_size(file_list_, LV_PCT(100), LV_PCT(100));
        lv_obj_set_flex_grow(file_list_, 1);
    }
    
    /**
     * @brief Refresh directory contents
     */
    void refreshDirectory() {
        current_entries_.clear();
        lv_obj_clean(file_list_);
        
        // Update breadcrumb
        lv_label_set_text(breadcrumb_label_, current_path_.c_str());
        
        #ifdef ESP32_BUILD
            refreshDirectoryESP32();
        #else
            refreshDirectoryDesktop();
        #endif
        
        // Create list items
        for (const auto& entry : current_entries_) {
            lv_obj_t* btn = lv_list_add_btn(file_list_, 
                entry.is_directory ? LV_SYMBOL_DIRECTORY : LV_SYMBOL_FILE,
                entry.name.c_str());
            lv_obj_add_event_cb(btn, fileItemCallback, LV_EVENT_CLICKED, this);
            
            // Store entry index in user data
            lv_obj_set_user_data(btn, (void*)(current_entries_.size() - 1));
        }
        
        // Notify directory changed
        if (dir_changed_cb_) {
            dir_changed_cb_(current_path_);
        }
    }
    
    #ifdef ESP32_BUILD
    /**
     * @brief ESP32-specific directory reading (supports both SPIFFS and SD)
     */
    void refreshDirectoryESP32() {
        // Determine which filesystem to use based on path
        FS* filesystem = nullptr;
        std::string real_path = current_path_;
        
        if (current_path_.substr(0, 7) == "/spiffs" || current_path_ == "/spiffs") {
            filesystem = &SPIFFS;
            real_path = current_path_.substr(7);  // Remove "/spiffs" prefix
            if (real_path.empty()) real_path = "/";
        } else if (current_path_.substr(0, 3) == "/sd" || current_path_ == "/sd") {
            filesystem = &SD;
            real_path = current_path_.substr(3);  // Remove "/sd" prefix
            if (real_path.empty()) real_path = "/";
        } else {
            // Root level - show filesystem options
            current_entries_.emplace_back("spiffs", "/spiffs", true, 0);
            if (SD.begin()) {
                current_entries_.emplace_back("sd", "/sd", true, 0);
            }
            return;
        }
        
        if (!filesystem) return;
        
        File root = filesystem->open(real_path.c_str());
        if (!root || !root.isDirectory()) {
            LV_LOG_WARN("Cannot open directory: %s", real_path.c_str());
            return;
        }
        
        File file = root.openNextFile();
        while (file) {
            std::string name = file.name();
            bool is_dir = file.isDirectory();
            size_t size = file.size();
            
            // Remove path prefix if present in name
            size_t last_slash = name.find_last_of('/');
            if (last_slash != std::string::npos) {
                name = name.substr(last_slash + 1);
            }
            
            if (shouldIncludeFile(name, is_dir)) {
                std::string full_path = current_path_;
                if (current_path_.back() != '/') full_path += "/";
                full_path += name;
                
                current_entries_.emplace_back(name, full_path, is_dir, size);
            }
            
            file = root.openNextFile();
        }
        root.close();
    }
    #else
    /**
     * @brief Desktop-specific directory reading
     */
    void refreshDirectoryDesktop() {
        try {
            for (const auto& entry : fs::directory_iterator(current_path_)) {
                std::string name = entry.path().filename().string();
                bool is_dir = entry.is_directory();
                size_t size = is_dir ? 0 : entry.file_size();
                
                if (shouldIncludeFile(name, is_dir)) {
                    current_entries_.emplace_back(name, 
                        entry.path().string(), is_dir, size);
                }
            }
        } catch (const std::exception& e) {
            LV_LOG_WARN("Directory read error: %s", e.what());
        }
    }
    #endif
    
    /**
     * @brief Check if file should be included based on filters
     */
    bool shouldIncludeFile(const std::string& name, bool is_directory) {
        // Always include directories
        if (is_directory) return true;
        
        // Skip hidden files
        if (name[0] == '.') return false;
        
        // Check file extension filters
        for (const auto& filter : file_filters_) {
            if (name.size() >= filter.size()) {
                std::string ext = name.substr(name.size() - filter.size());
                if (ext == filter) return true;
            }
        }
        
        return false;
    }
    
    /**
     * @brief Navigate up one directory level
     */
    void navigateUp() {
        #ifdef ESP32_BUILD
            if (current_path_ == "/" || current_path_.empty()) {
                return; // Already at root
            }
            
            // Handle filesystem root levels
            if (current_path_ == "/spiffs" || current_path_ == "/sd") {
                current_path_ = "/";
                refreshDirectory();
                return;
            }
            
            // Normal directory navigation
            size_t pos = current_path_.find_last_of('/');
            if (pos != std::string::npos && pos > 0) {
                current_path_ = current_path_.substr(0, pos);
                
                // Ensure we don't go above filesystem roots
                if (current_path_ == "/spiffs" || current_path_ == "/sd") {
                    // Stay at filesystem root
                } else if (current_path_.length() < 3) {
                    current_path_ = "/";
                }
            } else {
                current_path_ = "/";
            }
            refreshDirectory();
        #else
            fs::path current(current_path_);
            if (current.has_parent_path()) {
                current_path_ = current.parent_path().string();
                refreshDirectory();
            }
        #endif
    }
    
    // Event callbacks
    static void upButtonCallback(lv_event_t* e) {
        auto* browser = static_cast<LVGLFileBrowser*>(lv_event_get_user_data(e));
        browser->navigateUp();
    }
    
    static void fileItemCallback(lv_event_t* e) {
        auto* browser = static_cast<LVGLFileBrowser*>(lv_event_get_user_data(e));
        lv_obj_t* btn = lv_event_get_target(e);
        size_t index = (size_t)lv_obj_get_user_data(btn);
        
        if (index < browser->current_entries_.size()) {
            const auto& entry = browser->current_entries_[index];
            
            if (entry.is_directory) {
                browser->current_path_ = entry.full_path;
                browser->refreshDirectory();
            } else if (browser->file_selected_cb_) {
                browser->file_selected_cb_(entry);
            }
        }
    }
};

/**
 * @brief Example usage for MIDI file browser
 */
class MIDIFileBrowser {
private:
    std::unique_ptr<LVGLFileBrowser> browser_;
    
public:
    MIDIFileBrowser(lv_obj_t* parent) {
        browser_ = std::make_unique<LVGLFileBrowser>(parent);
        
        // Set MIDI-specific filters
        browser_->setFileFilters({".mid", ".midi", ".json"});
        
        // Set callbacks
        browser_->setFileSelectedCallback([this](const FileEntry& file) {
            handleFileSelected(file);
        });
        
        browser_->setDirectoryChangedCallback([this](const std::string& path) {
            LV_LOG_INFO("Changed to directory: %s", path.c_str());
        });
    }
    
private:
    void handleFileSelected(const FileEntry& file) {
        LV_LOG_INFO("Selected file: %s (size: %zu bytes)", 
                   file.name.c_str(), file.size);
        
        if (file.extension == "mid" || file.extension == "midi") {
            // Load MIDI file
            loadMIDIFile(file.full_path);
        } else if (file.extension == "json") {
            // Load configuration file
            loadConfigFile(file.full_path);
        }
    }
    
    void loadMIDIFile(const std::string& path) {
        // Implement MIDI file loading
        LV_LOG_INFO("Loading MIDI file: %s", path.c_str());
    }
    
    void loadConfigFile(const std::string& path) {
        // Implement config file loading
        LV_LOG_INFO("Loading config file: %s", path.c_str());
    }
};
