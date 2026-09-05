#pragma once
#include <Arduino.h>

// Shared hand-off between LogListScreen (picks a log) and LogSummaryScreen
// (renders it). The list sets the selected file before navigating; the
// summary reads it in onEnter(). Singleton namespace-scoped storage keeps the
// two screens decoupled from the UIManager/App screen registry.
void logViewerSelect(const char* file);
String logViewerSelected();