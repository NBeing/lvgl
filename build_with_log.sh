#!/bin/bash
# Build script that captures full output to build.log

echo "🧹 Cleaning previous build log..."
rm -f build.log

echo "🔨 Building ESP32 environment with full logging..."
pio run -e rymcu-esp32-s3-devkitc-1 2>&1 | tee build.log

echo ""
echo "📝 Build log saved to build.log"
echo "📊 Build result:"
if [ ${PIPESTATUS[0]} -eq 0 ]; then
    echo "✅ Build SUCCESS"
else
    echo "❌ Build FAILED"
    echo ""
    echo "🔍 Last 20 lines of build.log:"
    tail -20 build.log
fi
