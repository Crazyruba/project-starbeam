// display_v3.h - V3 Display Extension for Project Starbeam
// Drop-in addition for V2 display to support BRIDGE_STATUS menu item
//
// Usage: In your main .ino, after including display.h/display.cpp,
// call DisplayV3::init() once in setup() to register the extended labels.
//
// This file patches the menuLabels array to include the V3 BRIDGE_STATUS item.

#ifndef DISPLAY_V3_H
#define DISPLAY_V3_H

#include "display.h"

// ============================================================================
// Extended Menu Labels for V3
// Must match MenuItem enum in types.h exactly
// ============================================================================

class DisplayV3 {
public:
    // Call this in setup() after Display::init() to switch to V3 labels
    static void init();
    
    // Get the label for a menu item (handles V3 items)
    static const char* getMenuLabel(MenuItem item);
    
    // Redraw menu with V3 labels (use instead of Display::drawMenu)
    static void drawMenu(MenuItem selected, int firstVisible);
    
    // V3-specific displays
    static void displayBridgeStatus(BridgeState state, bool pingOk);
    static void displayV3BootScreen();
    
private:
    static bool v3LabelsActive;
};

#endif // DISPLAY_V3_H
