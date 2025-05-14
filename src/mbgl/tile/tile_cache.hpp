#pragma once

#include <mbgl/actor/scheduler.hpp>
#include <mbgl/tile/tile_id.hpp>
#include <mbgl/tile/tile.hpp>

#include <list>
#include <memory>
#include <map>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace mbgl {

class TileCache {
public:
    TileCache(const TaggedScheduler& threadPool_, size_t size_ = 0)
        : threadPool(threadPool_),
          size(size_) {}
    ~TileCache();

    /// Change the maximum size of the cache.
    void setSize(size_t);

    /// Get the maximum size
    size_t getMaxSize() const { return size; }

    /// Add a new tile with the given ID.
    /// If a tile with the same ID is already present, it will be retained and the new one will be discarded.
    void add(const OverscaledTileID& key, std::unique_ptr<Tile>&& tile);

    std::unique_ptr<Tile> pop(const OverscaledTileID& key);
    Tile* get(const OverscaledTileID& key);
    bool has(const OverscaledTileID& key);
    void clear();

    /// Set aside a tile to be destroyed later, without blocking
    void deferredRelease(std::unique_ptr<Tile>&&);

    /// Schedule any accumulated deferred tiles to be destroyed
    void deferPendingReleases();

    void updateSizeRange(size_t minSize_, size_t maxSize_) {
        minSize = minSize_;
        maxSize = maxSize_;
    }

    void setSource(std::string source_) { source = std::move(source_); }

    const std::string& getSource() const noexcept { return source; }

private:
    std::map<OverscaledTileID, std::unique_ptr<Tile>> tiles;
    std::list<OverscaledTileID> orderedKeys;
    TaggedScheduler threadPool;
    std::vector<std::unique_ptr<Tile>> pendingReleases;
    size_t deferredDeletionsPending{0};
    std::mutex deferredSignalLock;
    std::condition_variable deferredSignal;
    size_t size;
    size_t minSize = 0;
    size_t maxSize = 65536;
    std::string source = "";
};

} // namespace mbgl
