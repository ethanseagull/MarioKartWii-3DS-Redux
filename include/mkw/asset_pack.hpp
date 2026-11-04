#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace mkw {
// Indexed archive of menu runtime assets (tools/pack_menu_assets.py). One
// handle stays open, so each asset costs a seek and a read rather than a file
// open and close. Paths not in the archive report false; callers then use the
// loose file. Thread-safe: the asset loader and main thread may both read.
namespace pack {
// Opens menu.pak/race.pak once per session.
void open();
void close();
bool contains(const char* path);
// Reads the whole asset into out (resized). False if absent or unreadable.
bool read(const char* path,std::vector<uint8_t>& out);
// Size of an asset, or -1 if absent.
long size(const char* path);
// Reads exactly `bytes` (must equal size(path)) into destination.
bool readInto(const char* path,void* destination,size_t bytes);
// Reads `bytes` starting `offset` bytes into the asset; false past its end.
bool readRange(const char* path,size_t offset,void* destination,size_t bytes);
// Race-load RAM cache (asset_pack.cpp): the races of a VS set or Grand Prix
// re-read the same racers, items, effects and pause menu textures. enableCache sets the most it
// may keep (0: off; cacheLimitFor gives it for a heap size). While
// cacheReads(true), whole pack entries read are kept; kept entries are
// always served from RAM. trimCache, after a load, drops entries that load
// didn't read and returns the bytes kept; dropCache frees them all.
void enableCache(size_t limit);
size_t cacheLimitFor(size_t heapBytes);
void cacheReads(bool on);
size_t trimCache();
void dropCache();
#ifndef MKW_RELEASE
// DIAG: SD read speed test on race.pak (read-bench.txt). Writes one line per
// test to resultPath and passes each line to log. Mode 1: chunk sizes,
// threads, LZ11 decode. Mode 2: sector alignment, file depth, real entries.
void readBench(const char* resultPath,void(*log)(const char*),int mode);
#endif
}
}
