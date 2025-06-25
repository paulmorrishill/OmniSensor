#ifndef VERSION_H
#define VERSION_H

// Auto-generated version information
// This file is updated automatically during build

#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 0
#define FIRMWARE_VERSION_PATCH 0
#define FIRMWARE_BUILD_NUMBER 2

// Stringify macros
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// Combined version string
#define FIRMWARE_VERSION TOSTRING(FIRMWARE_VERSION_MAJOR) "." TOSTRING(FIRMWARE_VERSION_MINOR) "." TOSTRING(FIRMWARE_VERSION_PATCH) "." TOSTRING(FIRMWARE_BUILD_NUMBER)

#endif // VERSION_H