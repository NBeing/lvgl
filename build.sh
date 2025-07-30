#!/bin/bash
# Fast build helper script

set -e

echo "🚀 LVGL Synthesizer Build Helper"
echo "================================="

# Function to show usage
show_help() {
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  fast       - Ultra-fast development build (desktop, minimal features)"
    echo "  desktop    - Full desktop build with optimizations"
    echo "  esp32      - ESP32 release build"
    echo "  clean      - Clean all build artifacts"
    echo "  monitor    - Start MIDI monitoring"
    echo "  upload     - Build and upload to ESP32"
    echo ""
    echo "Examples:"
    echo "  $0 fast      # Quick iteration during development"
    echo "  $0 desktop   # Full desktop build for testing"
    echo "  $0 upload    # Deploy to ESP32 hardware"
}

# Parse command
case "${1:-desktop}" in
    "fast")
        echo "⚡ Ultra-fast development build..."
        pio run -e desktop-fast
        echo "✅ Fast build complete!"
        echo "💡 Run with: ./.pio/build/desktop-fast/program"
        ;;
    
    "desktop")
        echo "🖥️  Full desktop build..."
        pio run -e desktop
        echo "✅ Desktop build complete!"
        echo "💡 Run with: ./.pio/build/desktop/program"
        ;;
    
    "esp32")
        echo "📟 ESP32 build..."
        pio run -e rymcu-esp32-s3-arduino3x
        echo "✅ ESP32 build complete!"
        ;;
    
    "upload")
        echo "📤 Building and uploading to ESP32..."
        pio run -e rymcu-esp32-s3-arduino3x -t upload
        echo "✅ Upload complete!"
        ;;
    
    "clean")
        echo "🧹 Cleaning build artifacts..."
        pio run -t clean
        rm -rf .pio/build/*/
        echo "✅ Clean complete!"
        ;;
    
    "monitor")
        echo "📡 Starting MIDI monitor..."
        ./build_monitor.sh
        ;;
    
    "help"|"-h"|"--help")
        show_help
        ;;
    
    *)
        echo "❌ Unknown command: $1"
        echo ""
        show_help
        exit 1
        ;;
esac
