#pragma once

// The main plugin gets these headers transitively through PCH.h. The test target has
// no PCH of its own, so we supply the minimum subset the shared source files
// (XmlReader.h → hdtBulletHelper.h, hdtTemplateDefaults.cpp) implicitly depend on.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>  // BYTE            — XMLReader's buffer constructor
#include <algorithm>  // std::clamp      — hdtBulletHelper.h
#include <atomic>     // std::atomic     — hdt::SpinLock (hdtBulletHelper.h)
#include <bit>        // std::bit_floor  — hdtBulletHelper.h
#include <cstdint>    // std::uint*_t    — hdtBulletHelper.h
#include <mutex>      // std::lock_guard — hdtBulletHelper.h
#include <vector>     // std::vector     — hdt::vectorA16 alias (hdtBulletHelper.h)
