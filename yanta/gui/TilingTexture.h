#ifndef YANTA_TILLING_TEXTURE_H__
#define YANTA_TILLING_TEXTURE_H__

#include <deque>

#include <boost/thread.hpp>

#include <util/point.hpp>
#include <util/rect.hpp>
#include <util/Logger.h>
#include <gui/Texture.h>

extern logger::LogChannel tilingtexturelog;

/**
 * A texture used for prefetching drawings of a rasterizer. The texture's 
 * content can efficiently be shifted so that a different part is in the center.  
 * This may invalidate some of the texture's tiles, which have to be redrawn 
 * using updateDirtyRegions().
 */
class TilingTexture {

	// the size of a tile
	static const unsigned int TileSize = 32;

	// the number of tiles in the x and y direction
	static const unsigned int TilesX = 128;
	static const unsigned int TilesY = 128;

public:

	/**
	 * Create a new prefetch texture around the given point.
	 */
	TilingTexture(const util::point<int>& center);

	~TilingTexture();

	/**
	 * Shift the region represented by this prefetch texture. This method is 
	 * lightweight -- it only remembers the new position and marks some regions 
	 * as dirty.
	 */
	void shift(const util::point<int>& shift);

	/**
	 * Reset the texture around center. This will mark all tiles dirty.
	 */
	void reset(const util::point<int>& center);

	/**
	 * Update a region of the texture with whatever the provided rasterizer 
	 * draws into this region. The previous content of the texture will be 
	 * available to the rasterizer, i.e., alpha operations can be performed on 
	 * top of the previous content.
	 */
	template <typename Rasterizer>
	void update(
			const util::rect<int>& region,
			Rasterizer&            rasterizer);

	/**
	 * Render the texture's content of region to region on the screen.
	 */
	void render(const util::rect<int>& region);

	/**
	 * Mark a region within the texture as dirty. Thread safe.
	 */
	void markDirty(const util::rect<int>& region);

	/**
	 * Check whether there are dirty regions in the texture.
	 */
	bool hasDirtyRegions() { return !_cleanUpRequests.empty(); }

	/**
	 * Clean dirty regions. At most maxNumRequests are processed. Returns the 
	 * number of processed requests.
	 */
	template <typename Rasterizer>
	unsigned int cleanUp(Rasterizer& rasterizer, unsigned int maxNumRequests = 0);

	/**
	 * Check whether a given region is dirty.
	 */
	bool isDirty(const util::rect<int>& region);

	/**
	 * Get the width of the region represented by this texture.
	 */
	unsigned int width() { return _tilesRegion.width(); }

	/**
	 * Get the height of the region represented by this texture.
	 */
	unsigned int height() { return _tilesRegion.height(); }

private:

	struct CleanUpRequest {

		CleanUpRequest() {};

		CleanUpRequest(
				const util::point<int>& tile_,
				const util::rect<int>& tileRegion_) :
				tile(tile_),
				tileRegion(tileRegion_) {}

		util::point<int> tile;
		util::rect<int>  tileRegion;
	};

	/**
	 * Mark a tile dirty.
	 */
	void markDirty(const util::point<int>& tile);

	/**
	 * Get the next cleanup request. Returns false, if there are none. Thread 
	 * safe.
	 */
	bool getNextCleanUpRequest(CleanUpRequest& request);

	/**
	 * Update a tile, using rasterizer, but draw only in the given roi.
	 */
	template <typename Rasterizer>
	void updateTile(
			const util::point<int>& tileInArray,
			Rasterizer&             rasterizer,
			const util::rect<int>&  roi);

	/**
	 * Update a tile, using rasterizer, but draw only in the given roi. Use the 
	 * given buffer instead of the tile's texture buffer.
	 */
	template <typename Rasterizer>
	void updateTile(
			const util::point<int>& tileInArray,
			Rasterizer&             rasterizer,
			const util::rect<int>&  roi,
			gui::Buffer&            buffer);

	/**
	 * Rasterize into bufferData, representing bufferRegion, limited to roi, 
	 * using rasterizer.
	 */
	template <typename Rasterizer>
	void rasterize(
			Rasterizer&            rasterizer,
			gui::skia_pixel_t*     bufferData,
			const util::rect<int>& bufferRegion,
			const util::rect<int>& roi);

	/**
	 * Split a region into four parts, according to the beginning of the content 
	 * in the tiles array. Additionally, give the tiles that intersect the parts 
	 * and the content offset for each part.
	 */
	void split(
		const util::rect<int>& region,
		util::rect<int>*       subregions);

	/**
	 * Get all the tiles intersecting region.
	 */
	util::rect<int> getTiles(const util::rect<int>& region);

	/**
	 * Get the region covered by a tile.
	 */
	util::rect<int> getTileRegion(const util::point<int>& tile);

	// the whole region (including prefetch) covered by the tiles in pixel units
	util::rect<int> _tilesRegion;

	// accumulated shifts
	util::point<int> _shift;

	// the OpenGl textures to draw to
	gui::Texture* _tiles[TilesX][TilesY];

	// the top-left tile in content space
	util::point<int> _topLeftTile;

	// dirty tiles of the texture
	std::deque<CleanUpRequest> _cleanUpRequests;

	// a clean-up buffer per thread
	boost::thread_specific_ptr<gui::Buffer> _cleanUpBuffer;
};

template <typename Rasterizer>
void
TilingTexture::update(
		const util::rect<int>& region,
		Rasterizer&            rasterizer) {

	LOG_DEBUG(tilingtexturelog) << "updating region " << region << std::endl;

	// get the up-to-four parts of the region in tiles
	util::rect<int>  subregions[4];
	split(region, subregions);

	// for each part...
	for (int i = 0; i < 4; i++) {

		// ...if not empty...
		if (subregions[i].area() <= 0)
			continue;

		util::rect<int> tiles = getTiles(subregions[i]);

		LOG_ALL(tilingtexturelog) << "  updating tiles in " << tiles << std::endl;

		// ...for each tile in this part...
		for (int x = tiles.minX; x < tiles.maxX; x++) {
			for (int y = tiles.minY; y < tiles.maxY; y++) {

				util::point<int> tileInArray(x, y);

				// ...update it with subregions[i] as roi
				updateTile(tileInArray, rasterizer, subregions[i]);
			}
		}
	}
}

template <typename Rasterizer>
void
TilingTexture::updateTile(
		const util::point<int>& tileInArray,
		Rasterizer&             rasterizer,
		const util::rect<int>&  roi) {

	LOG_DEBUG(tilingtexturelog) << "updating tile " << tileInArray << std::endl;

	// map the texture directly
	gui::skia_pixel_t* data = _tiles[tileInArray.x][tileInArray.y]->map<gui::skia_pixel_t>();

	// draw to it
	rasterize(rasterizer, data, getTileRegion(tileInArray), roi);

	// unmap the texture
	_tiles[tileInArray.x][tileInArray.y]->unmap<gui::skia_pixel_t>();
}

template <typename Rasterizer>
void
TilingTexture::updateTile(
		const util::point<int>& tileInArray,
		Rasterizer&             rasterizer,
		const util::rect<int>&  roi,
		gui::Buffer&            buffer) {

	LOG_DEBUG(tilingtexturelog) << "updating tile " << tileInArray << std::endl;

	// map the buffer
	gui::skia_pixel_t* data = buffer.map<gui::skia_pixel_t>();

	// draw to it
	rasterize(rasterizer, data, getTileRegion(tileInArray), roi);

	// unmap the buffer
	buffer.unmap();

	// reload tile texture
	_tiles[tileInArray.x][tileInArray.y]->loadData(buffer);
}

template <typename Rasterizer>
void
TilingTexture::rasterize(
		Rasterizer&            rasterizer,
		gui::skia_pixel_t*     bufferData,
		const util::rect<int>& bufferRegion,
		const util::rect<int>& roi) {

	// wrap the buffer in a skia bitmap
	SkBitmap bitmap;
	bitmap.setConfig(SkBitmap::kARGB_8888_Config, bufferRegion.width(), bufferRegion.height());
	bitmap.setPixels(bufferData);

	SkCanvas canvas(bitmap);

	// Now, we have a surface of widthxheight, with (0,0) being the upper left 
	// corner and (width-1,height-1) the lower right. Translate operations, such 
	// that the upper left is bufferRegion.upperLeft() and lower right is 
	// bufferRegion.lowerRight().

	// translate bufferArea.upperLeft() to (0,0)
	util::point<int> translate = -bufferRegion.upperLeft();
	canvas.translate(translate.x, translate.y);

	rasterizer.draw(canvas, roi);
}

template <typename Rasterizer>
unsigned int
TilingTexture::cleanUp(Rasterizer& rasterizer, unsigned int maxNumRequests) {

	gui::OpenGl::Guard guard;

	if (_cleanUpBuffer.get() == 0) {

		GLenum format = gui::detail::pixel_format_traits<gui::skia_pixel_t>::gl_format;
		GLenum type   = gui::detail::pixel_format_traits<gui::skia_pixel_t>::gl_type;

		_cleanUpBuffer.reset(new gui::Buffer(TileSize, TileSize, format, type));
	}

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

		updateTile(request.tile, rasterizer, request.tileRegion, *_cleanUpBuffer);
	}

	return i;
}

#endif // YANTA_TILLING_TEXTURE_H__

