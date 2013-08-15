#include <boost/timer/timer.hpp>

#include "TilesCache.h"

logger::LogChannel tilescachelog("tilescachelog", "[TilesCache] ");

TilesCache::TilesCache(const util::point<int>& center) :
	_backgroundPainterStopped(false),
	_backgroundThread(boost::bind(&TilesCache::cleanUp, this)) {

	LOG_ALL(tilescachelog) << "creating new tiles cache around " << center << std::endl;

	reset(center);
}

TilesCache::~TilesCache() {

	LOG_ALL(tilescachelog) << "tearing background thread down..." << std::endl;

	_backgroundPainterStopped = true;
	_backgroundThread.join();

	LOG_ALL(tilescachelog) << "background thread stopped" << std::endl;
}

void
TilesCache::reset(const util::point<int>& center) {

	// reset the tile mapping, such that all tiles around center map to 
	// [0,w)x[0,h)
	_mapping.reset(center/static_cast<int>(TileSize) - util::point<int>(Width/2, Height/2));

	// reset accumulated shift
	_shift = util::point<int>(0, 0);

	// dismiss all pending clean-up requests
	_cleanUpRequests.clear();

	// mark all tiles dirty
	util::rect<int> tilesRegion = _mapping.get_region();
	for (int x = tilesRegion.minX; x < tilesRegion.maxX; x++)
		for (int y = tilesRegion.minY; y < tilesRegion.maxY; y++)
			markDirty(util::point<int>(x, y), NeedsRedraw);
}

void
TilesCache::shift(const util::point<int>& shift) {

	LOG_ALL(tilescachelog) << "shifting cache content by " << shift << std::endl;

	_shift += shift;

	// We are shifting content out of the region covered by this cache.
	//
	// If we shifted far enough to the right, such that a whole column of tiles 
	// got shifted out of the region, we take this column and insert it at the 
	// left. We mark the newly inserted tiles as dirty.

	while (_shift.x >= (int)TileSize) {

		_mapping.shift(util::point<int>(1, 0));
		_shift.x -= TileSize;

		// the new tiles are in the left column
		util::rect<int> tilesRegion = _mapping.get_region();
		int x = tilesRegion.minX;
		for (int y = tilesRegion.minY; y < tilesRegion.maxY; y++)
			markDirty(util::point<int>(x, y), NeedsRedraw);
	}
	while (_shift.x <= -(int)TileSize) {

		_mapping.shift(util::point<int>(-1, 0));
		_shift.x += TileSize;

		// the new tiles are in the right column
		util::rect<int> tilesRegion = _mapping.get_region();
		int x = tilesRegion.maxX;
		for (int y = tilesRegion.minY; y < tilesRegion.maxY; y++)
			markDirty(util::point<int>(x, y), NeedsRedraw);
	}
	while (_shift.y >= (int)TileSize) {

		_mapping.shift(util::point<int>(0, 1));
		_shift.y -= TileSize;

		// the new tiles are in the top column
		util::rect<int> tilesRegion = _mapping.get_region();
		int y = tilesRegion.minY;
		for (int x = tilesRegion.minX; x < tilesRegion.maxX; x++)
			markDirty(util::point<int>(x, y), NeedsRedraw);
	}
	while (_shift.y <= -(int)TileSize) {

		_mapping.shift(util::point<int>(0, -1));
		_shift.y += TileSize;

		// the new tiles are in the bottom column
		util::rect<int> tilesRegion = _mapping.get_region();
		int y = tilesRegion.maxY;
		for (int x = tilesRegion.minX; x < tilesRegion.maxX; x++)
			markDirty(util::point<int>(x, y), NeedsRedraw);
	}

	LOG_ALL(tilescachelog) << "  cache region is now " << _mapping.get_region() << std::endl;
}

void
TilesCache::markDirty(const util::point<int>& tile, TileState state) {

	LOG_ALL(tilescachelog) << "marking tile " << tile << " as " << (state == NeedsUpdate ? "needs update" : "needs redraw") << std::endl;

	util::point<int> physicalTile = _mapping.map(tile);

	_tileStates[physicalTile.x][physicalTile.y] = state;

	// pass only complete redraw requests to the background thread
	if (state == NeedsRedraw) {

		// get the region covered by the tile in pixels
		util::rect<int> tileRegion(tile.x, tile.y, tile.x + 1, tile.y + 1);
		tileRegion *= static_cast<int>(TileSize);

		// queue a clean-up request
		CleanUpRequest request(physicalTile, tileRegion);
		_cleanUpRequests.push_back(request);
	}
}

gui::skia_pixel_t*
TilesCache::getTile(const util::point<int>& tile, SkiaDocumentPainter& painter) {

	LOG_ALL(tilescachelog) << "getting tile " << tile << std::endl;

	util::point<int> physicalTile = _mapping.map(tile);

	if (_tileStates[physicalTile.x][physicalTile.y] == NeedsUpdate) {

		// get the region covered by the tile in pixels
		util::rect<int> tileRegion(tile.x, tile.y, tile.x + 1, tile.y + 1);
		tileRegion *= static_cast<int>(TileSize);

		updateTile(physicalTile, tileRegion, painter);
	}

	if (_tileStates[physicalTile.x][physicalTile.y] == NeedsRedraw) {

		// get the region covered by the tile in pixels
		util::rect<int> tileRegion(tile.x, tile.y, tile.x + 1, tile.y + 1);
		tileRegion *= static_cast<int>(TileSize);

		painter.setIncremental(false);
		updateTile(physicalTile, tileRegion, painter);
		painter.setIncremental(true);
	}

	return _tiles[physicalTile.x][physicalTile.y];
}

void
TilesCache::setBackgroundPainter(boost::shared_ptr<SkiaDocumentPainter> painter) {

	_backgroundPainter = painter;
}

void
TilesCache::updateTile(const util::point<int>& physicalTile, const util::rect<int>& tileRegion, SkiaDocumentPainter& painter) {

	gui::skia_pixel_t* buffer = _tiles[physicalTile.x][physicalTile.y];

	// wrap the buffer in a skia bitmap
	SkBitmap bitmap;
	bitmap.setConfig(SkBitmap::kARGB_8888_Config, TileSize, TileSize);
	bitmap.setPixels(buffer);

	SkCanvas canvas(bitmap);

	// Now, we have a surface of widthxheight, with (0,0) being the upper left 
	// corner and (width-1,height-1) the lower right. Translate operations, such 
	// that the upper left is tileRegion.upperLeft() and lower right is 
	// tileRegion.lowerRight().

	// translate bufferArea.upperLeft() to (0,0)
	util::point<int> translate = -tileRegion.upperLeft();
	canvas.translate(translate.x, translate.y);

	painter.draw(canvas, tileRegion);
}

void
TilesCache::cleanUp() {

	boost::timer::cpu_timer timer;

	const boost::timer::nanosecond_type NanosBusyWait = 100000LL;     // 1/10000th of a second
	const boost::timer::nanosecond_type NanosIdleWait = 1000000000LL; // 1/1th of a second

	bool isClean = false;

	while (!_backgroundPainterStopped) {

		// was there something to clean?
		isClean = (_backgroundPainter && cleanDirtyTiles(2) == 0);

		boost::timer::cpu_times const elapsed(timer.elapsed());

		// reduce poll time if everything is clean
		boost::timer::nanosecond_type waitAtLeast = (isClean ? NanosIdleWait : NanosBusyWait);

		if (elapsed.wall <= waitAtLeast)
			usleep((waitAtLeast - elapsed.wall)/1000);

		timer.stop();
		timer.start();
	}
}

unsigned int
TilesCache::cleanDirtyTiles(unsigned int  maxNumRequests) {

	unsigned int numRequests = _cleanUpRequests.size();
	
	if (maxNumRequests != 0)
		numRequests = std::min(numRequests, maxNumRequests);

	CleanUpRequest request;

	unsigned int i = 0;
	for (; i < numRequests; i++) {

		// the number of requests might have decreased while we were working on 
		// them, in this case abort and come back later
		if (!getNextCleanUpRequest(request))
			return i;

		updateTile(request.tile, request.tileRegion, *_backgroundPainter);
	}

	return i;
}

bool
TilesCache::getNextCleanUpRequest(CleanUpRequest& request) {

	if (_cleanUpRequests.size() == 0)
		return false;

	request = _cleanUpRequests.front();
	_cleanUpRequests.pop_front();

	return true;
}
