#ifndef SPLINES_PREFETCH_TEXTURE_H__
#define SPLINES_PREFETCH_TEXTURE_H__

#include <cairo.h>

#include <gui/Texture.h>

#include "CairoCanvasPainter.h"

/**
 * A texture used for prefetching drawings of a cairo painter. The texture's 
 * content can efficiently be shifted so that a different part is in the center.  
 * This invalidates some of the texture's areas, which have to be redrawn using 
 * fill.
 */
class PrefetchTexture {

public:

	/**
	 * Create a new prefetch texture for the given area and prefetch values in 
	 * pixels for each direction.
	 */
	PrefetchTexture(
			const util::rect<int>& area,
			unsigned int prefetchLeft,
			unsigned int prefetchRight,
			unsigned int prefetchTop,
			unsigned int prefetchBottom);

	/**
	 * Shift the area represented by this prefetch texture. This method is 
	 * lightweight -- it only remembers the new position and marks some subareas 
	 * as dirty.
	 */
	void shift(const util::point<int>& shift);

	/**
	 * Fill a subarea of the texture with whatever the provided painter draws 
	 * into this area.
	 */
	void fill(
			const util::rect<int>& subarea,
			CairoCanvasPainter& painter);

	/**
	 * Render the texture's content of subarea to subarea on the screen.
	 */
	void render(const util::rect<int>& roi, const util::point<double>& _deviceUnitsPerPixel, const util::point<double>& _deviceOffset);

	/**
	 * Get a vector of all dirty areas.
	 */
	const std::vector<util::rect<int> >& getDirtyAreas();

	/**
	 * Process all dirty areas and clean them.
	 */
	void cleanDirtyAreas(CairoCanvasPainter& painter);

	/**
	 * Put the split point to the upper left corner.
	 */
	void reset(const util::rect<int>& area);

	unsigned int width() { return _textureArea.width(); }
	unsigned int height() { return _textureArea.height(); }

private:

	/**
	 * Prepare the texture and buffers of the respective sizes. Returns true, if 
	 * a new texture was loaded.
	 */
	bool prepare();

	/**
	 * Mark an area within the texture as dirty.
	 */
	void markDirty(const util::rect<int>& area);

	// the whole area (including prefetch) covered by the texture in pixel units
	util::rect<int> _textureArea;

	// the number of pixels used for prefetching
	unsigned int _prefetchLeft;
	unsigned int _prefetchRight;
	unsigned int _prefetchTop;
	unsigned int _prefetchBottom;

	// the width of the x-, and the height of the y-buffer
	unsigned int _bufferWidth;
	unsigned int _bufferHeight;

	// the OpenGl texture to draw to
	gui::Texture* _texture;

	// OpenGl buffers to reload dirty parts of the texture
	gui::Buffer* _reloadBufferX;
	gui::Buffer* _reloadBufferY;

	// the split center of the texture (where all four corners meet)
	util::point<int> _splitCenter;

	// dirty areas of the texture
	std::vector<util::rect<int> > _dirtyAreas;
};

#endif // SPLINES_PREFETCH_TEXTURE_H__

