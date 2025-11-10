/**
 * @file version.h
 * @brief Version information for UWB Scanner
 */

#ifndef VERSION_H
#define VERSION_H

/* Application version */
#define APP_VERSION_MAJOR 1
#define APP_VERSION_MINOR 0
#define APP_VERSION_PATCH 0

#define APP_VERSION_STRING "1.0.0"

/* Git information - will be populated by CMake */
#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH "unknown"
#endif

#ifndef BUILD_TIMESTAMP
#define BUILD_TIMESTAMP __DATE__ " " __TIME__
#endif

#endif /* VERSION_H */
