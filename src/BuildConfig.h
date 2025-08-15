#pragma once

// Build configuration flags for gradual integration

// Choose between old and new architecture - use build flag if available
#ifndef USE_THREADED_ARCHITECTURE
    #define USE_THREADED_ARCHITECTURE 0  // Default to old architecture if not defined in build
#endif

// Gradual integration flags
#define ENABLE_ALL_TABS 1            // Set to 1 to enable all tabs (Main, Hello, World, etc.)
#define ENABLE_THREADED_MIDI 1       // Set to 1 to enable threaded MIDI processing
