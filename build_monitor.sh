#!/bin/bash

# Build monitoring script with comprehensive logging
LOG_FILE="build_$(date +%Y%m%d_%H%M%S).log"
echo "🚀 Starting build at $(date)" | tee -a $LOG_FILE
echo "============================================" | tee -a $LOG_FILE

# Run build with full output capture
pio run -e rymcu-esp32-s3-arduino3x 2>&1 | tee -a $LOG_FILE

# Check exit status
if [ ${PIPESTATUS[0]} -eq 0 ]; then
    echo "✅ Build SUCCESSFUL at $(date)" | tee -a $LOG_FILE
    echo "🎯 Binary location: .pio/build/rymcu-esp32-s3-arduino3x/" | tee -a $LOG_FILE
else
    echo "❌ Build FAILED at $(date)" | tee -a $LOG_FILE
    echo "🔍 Check errors above" | tee -a $LOG_FILE
fi

echo "============================================" | tee -a $LOG_FILE
echo "📋 Log saved to: $LOG_FILE"
