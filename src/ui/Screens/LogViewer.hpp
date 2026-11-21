#pragma once

#include <Arduino.h>

// Shared hand-off between LogListScreen and LogSummaryScreen. The user picks a
// log on LogListScreen; LogSummaryScreen reads the same path to parse and
// (optionally) delete it. Declared here (rather than an anonymous namespace in
// one .cpp) so both translation units link against the same object.
extern String g_selectedLog;

inline void logViewerSelect(const char* file) { g_selectedLog = file ? file : ""; }
inline String logViewerSelected() { return g_selectedLog; }
