#ifndef YANTA_GUI_TILES_CACHE_H__
#define YANTA_GUI_TILES_CACHE_H__

#include <boost/multi_array.hpp>
#include <boost/thread.hpp>

#include <util/torus_mapping.hpp>
#include "SkiaDocumentPainter.h"

/**
 * Stores tiles (rectangle image buffers) on a torus topology to have them 
 * quickly available for drawing.
 */
class TilesCache {

public:

	// the size of a tile
	static const unsigned int TileSize = 128;

	// the number of tiles in the x and y direction
	static const unsigned int Width  = 16;
	static const unsigned int Height = 16;

	/**
	 * The possible state of tiles in the cache.
	 */
	enum TileState {

		// the tile is clean and ready for use
		Clean,

		// the tile needs a possibly incremental update
		NeedsUpdate,

		// the tile needs to be redrawn completely
		NeedsRedraw
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
	 * will be updated using the provided painter. The caller has to ensure that 
	 * the tile is part of the cache.
	 */
	gui::skia_pixel_t* getTile(const util::point<int>& tile, SkiaDocumentPainter& painter);

	/**
	 * Set a background painter for this cache. This will launch a background 
	 * thread that is periodically cleaning dirty tiles.
	 */
	void setBackgroundPainter(boost::shared_ptr<SkiaDocumentPainter> painter);

private:

	/**
	 * A clean-up request for the background thread. Informs the thread about 
	 * which tile has to be redrawn with which content.
	 */
	struct CleanUpRequest {

		CleanUpRequest() {};

		CleanUpRequest(
				const util::point<int>& tile_,
				const util::rect<int>& tileRegion_) :
				tile(tile_),
				tileRegion(tileRegion_) {}

		// physical coordinates of the tile to update
		util::point<int> tile;

		// region of the tile in pixels
		util::rect<int>  tileRegion;
	};

	/**
	 * Update a tile.
	 *
	 * @param physicalTile
	 *              The physical coordinates of the tile.
	 * @param tileRegion
	 *              The region covered by the tile in pixels.
	 * @param painter
	 *              The painter to use.
	 */
	void updateTile(const util::point<int>& physicalTile, const util::rect<int>& tileRegion, SkiaDocumentPainter& painter);

	/**
	 * Entry point of the background thread.
	 */
	void cleanUp();

	/**
	 * Clean at most maxNumRequests dirty tiles.
	 */
	unsigned int cleanDirtyTiles(unsigned int maxNumRequests);

	/**
	 * Get the next clean-up request from the queue. Returns false, if there are 
	 * none.
	 */
	bool getNextCleanUpRequest(CleanUpRequest& request);

	// 2D array of tiles
	typedef boost::multi_array<gui::skia_pixel_t, 3> tiles_type;
	tiles_type  _tiles;

	// 2D array of states for the tiles
	typedef boost::multi_array<TileState, 2> tile_states_type;
	tile_states_type _tileStates;

	// mapping from logical tile coordinates to physical coordinates in 2D array
	torus_mapping<int, Width, Height> _mapping;

	// queue of clean-up requests for the background thread
	std::deque<CleanUpRequest> _cleanUpRequests;

	// the painter to be used by the background thread
	boost::shared_ptr<SkiaDocumentPainter> _backgroundPainter;

	// used to stop the background rendering thread
	bool _backgroundPainterStopped;

	// the background rendering thread keeping dirty tiles clean
	boost::thread _backgroundThread;
};

#endif // YANTA_GUI_TILES_CACHE_H__

