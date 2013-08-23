#ifndef YANTA_GUI_TILES_CACHE_H__
#define YANTA_GUI_TILES_CACHE_H__

#include <boost/multi_array.hpp>
#include <boost/thread.hpp>

#include <gui/Skia.h>

#include <util/torus_mapping.hpp>
#include "Rasterizer.h"

/**
 * Stores tiles (rectangle image buffers) on a torus topology to have them 
 * quickly available for drawing.
 */
class TilesCache {

public:

	// the size of a tile
	static const unsigned int TileSize = 128;

	// the number of tiles in the x and y direction
	static const unsigned int Width  = 64;
	static const unsigned int Height = 64;

	/**
	 * The possible state of tiles in the cache.
	 */
	enum TileState {

		// the tile is clean and ready for use
		Clean,

		// the tile needs a possibly incremental update
		NeedsUpdate,

		// the tile needs to be redrawn completely
		NeedsRedraw,

		// the cache for this tile is invalid
		Invalid
	};

	/**
	 * Create a new cache with tile 'center' being in the middle.
	 *
	 * @param center
	 *              The logical coordinates of the center tile.
	 */
	TilesCache(const util::point<int>& center = util::point<int>(0, 0));

	~TilesCache();

	/**
	 * Reset the cache such that tile 'center' is in the middle. This will mark 
	 * all tiles dirty.
	 *
	 * @param center
	 *              The logical coordinates of the center tile.
	 */
	void reset(const util::point<int>& center);

	/**
	 * Shift the content of the cache. This method is lightweight,
	 * it only remembers the new position and marks some tiles as dirty.
	 *
	 * @param shift
	 *              The amount by which to shift the cache in tiles.
	 */
	void shift(const util::point<int>& shift);

	/**
	 * Mark a tile as dirty.
	 */
	void markDirty(const util::point<int>& tile, TileState state);

	/**
	 * Get the data of a tile in the cache. If the tile was marked dirty, it 
	 * will be updated using the provided rasterizer. The caller has to ensure 
	 * that the tile is part of the cache.
	 */
	gui::skia_pixel_t* getTile(const util::point<int>& tile, Rasterizer& rasterizer);

	/**
	 * Ask whether a tile was changed by the cache. If it was changed, the 
	 * caller has to indicate that the change was observed by calling 
	 * seenChange().
	 */
	bool wasChanged(const util::point<int>& tile);

	/**
	 * Indicate that the change indicated by wasChanged() was acknowledged by 
	 * the caller. Call this method before you reload the tile to make sure you 
	 * don't miss another change.
	 */
	void seenChange(const util::point<int>& tile);

	/**
	 * Set a background rasterizer for this cache. This will launch a background 
	 * thread that is periodically cleaning dirty tiles.
	 */
	void setBackgroundRasterizer(boost::shared_ptr<Rasterizer> rasterizer);

private:

	/**
	 * Set the dirty flag of a physical tile.
	 */
	inline void markDirtyPhysical(const util::point<int>& physicalTile, TileState state);

	/**
	 * Update a tile.
	 *
	 * @param physicalTile
	 *              The physical coordinates of the tile.
	 * @param tileRegion
	 *              The region covered by the tile in pixels.
	 * @param rasterizer
	 *              The rasterizer to use.
	 */
	void updateTile(const util::point<int>& physicalTile, const util::rect<int>& tileRegion, Rasterizer& rasterizer);

	/**
	 * Entry point of the background thread.
	 */
	void cleanUp();

	/**
	 * Clean at most maxNumRequests dirty tiles.
	 */
	unsigned int cleanDirtyTiles(unsigned int maxNumRequests);

	/**
	 * Check whether a logical tile is marked as invalid, get the physical tile 
	 * and the region covered by it on-the-fly.
	 */
	bool isInvalid(const util::point<int>& tile, util::point<int>& physicalTile, util::rect<int>& tileRegion);

	// 2D array of tiles
	typedef boost::multi_array<gui::skia_pixel_t, 3> tiles_type;
	tiles_type  _tiles;

	// 2D array of states for the tiles
	typedef boost::multi_array<TileState, 2> tile_states_type;
	tile_states_type _tileStates;

	// 2D array of changed-flags for the tiles
	typedef boost::multi_array<bool, 2> tile_changed_type;
	tile_changed_type _tileChanged;

	// 2D array of mutexes for the tiles
	boost::mutex _tileMutexes[Width][Height];

	// mapping from logical tile coordinates to physical coordinates in 2D array
	torus_mapping<int, Width, Height> _mapping;

	// mutex to protect the mapping
	boost::mutex _mappingMutex;

	// the rasterizer to be used by the background thread
	boost::shared_ptr<Rasterizer> _backgroundRasterizer;

	// used to stop the background rendering thread
	bool _backgroundRasterizerStopped;

	// the background rendering thread keeping dirty tiles clean
	boost::thread _backgroundThread;
};

#endif // YANTA_GUI_TILES_CACHE_H__

