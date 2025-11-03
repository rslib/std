#pragma once

/**
 * API export/import macros for rs_std shared library visibility.
 *
 * This header defines RS_STD_API for controlling symbol visibility:
 * - Windows: Uses __declspec(dllexport/dllimport)
 * - GCC/Clang: Uses __attribute__((visibility("default")))
 * - Static builds: No special annotation
 */

#include <rs/std/macros.h>

// ============================================================================
// API visibility macros
// ============================================================================

#if defined(RS_STD_STATIC)
// Static library - no export/import needed
#define RS_STD_API
#define RS_STD_API_HIDDEN

#elif defined(RS_PLATFORM_WINDOWS)
// Windows DLL
#if defined(RS_STD_EXPORTS)
// Building the DLL - export symbols
#define RS_STD_API __declspec(dllexport)
#else
// Using the DLL - import symbols
#define RS_STD_API __declspec(dllimport)
#endif
#define RS_STD_API_HIDDEN

#elif defined(__GNUC__) || defined(__clang__)
// GCC/Clang - use visibility attributes
#if defined(RS_STD_EXPORTS)
#define RS_STD_API __attribute__((visibility("default")))
#else
#define RS_STD_API
#endif
#define RS_STD_API_HIDDEN __attribute__((visibility("hidden")))

#else
// Unknown compiler - no special handling
#define RS_STD_API
#define RS_STD_API_HIDDEN

#endif
