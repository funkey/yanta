#ifndef YANTA_PREFETCH_TEXTURE_H__
#define YANTA_PREFETCH_TEXTURE_H__

#include <deque>
#include <boost/thread.hpp>

#include <cairo.h>

#include <gui/Texture.h>

#include "SkiaCanvasPainter.h"

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

	~PrefetchTexture();

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
			SkiaCanvasPainter& painter);

	/**
	 * Specify an area as working area. Buffers for this area will be kept in 
	 * memory and incremental drawing operations can be performed on it when 
	 * fill() is called with exactly this subarea.
	 */
	void setWorkingArea(const util::rect<int>& subarea);

	/**
	 * Render the texture's content of roi to roi on the screen.
	 */
	void render(const util::rect<int>& roi);

	/**
	 * Put the split point to the upper left corner.
	 */
	void reset(const util::rect<int>& area);

	/**
	 * Mark an area within the texture as dirty. Thread safe.
	 */
	void markDirty(const util::rect<int>& area);

	/**
	 * Check whether there are dirty areas in the texture.
	 */
	bool hasDirtyAreas() { return !_cleanUpRequests.empty(); }

	/**
	 * Clean all dirty areas.
	 */
	void cleanUp(SkiaCanvasPainter& painter, unsigned int maxNumRequests);

	/**
	 * Check whether a given area is dirty.
	 */
	bool isDirty(const util::rect<int>& roi);

	unsigned int width() { return _textureArea.width(); }
	unsigned int height() { return _textureArea.height(); }

private:

	struct CleanUpRequest {

		CleanUpRequest() {};

		CleanUpRequest(
				const util::rect<int>& area_,
				const util::point<int>& textureOffset_) :
				area(area_),
				textureOffset(textureOffset_) {}

		util::rect<int>  area;
		util::point<int> textureOffset;
	};

	/**
	 * Prepare the texture and buffers of the respective sizes. Returns true, if 
	 * a new texture was loaded.
	 */
	bool prepare();

	/**
	 * Get the next cleanup request. Returns false, if there are none. Thread 
	 * safe.
	 */
	bool getNextCleanUpRequest(CleanUpRequest& request);

	/**
	 * Fill buffer, representing bufferArea, using painter, but draw only in 
	 * roi.
	 */
	void fillBuffer(
			gui::Buffer&           buffer,
			const util::rect<int>& bufferArea,
			SkiaCanvasPainter&     painter,
			const util::rect<int>& roi);

	/**
	 * Split an area into four parts, according to the beginning of the content 
	 * in the texture. Additionally, give the offset for each part into the 
	 * texture.
	 */
	void split(const util::rect<int>& subarea, util::rect<int>* parts, util::point<int>* offsets);

	void createBuffer(unsigned int width, unsigned int height, gui::Buffer** buffer);

	void deleteBuffer(gui::Buffer** buffer);

	// the whole area (including prefetch) covered by the texture in pixel units
	util::rect<int> _textureArea;

	// the number of pixels used for prefetching
	unsigned int _prefetchLeft;
	unsigned int _prefetchRight;
	unsigned int _prefetchTop;
	unsigned int _prefetchBottom;

	// the OpenGl texture to draw to
	gui::Texture* _texture;

	// a mutex to protect access to _texture
	boost::mutex _textureMutex;

	// the width of the x-, and the height of the y-buffer (used to reload dirty 
	// areas along the edges of the texture)
	unsigned int _bufferWidth;
	unsigned int _bufferHeight;

	// OpenGl buffers to reload dirty parts of the texture
	gui::Buffer* _reloadBufferX;
	gui::Buffer* _reloadBufferY;

	// the point where all four corners of the texture meet in content space
	util::point<int> _splitPoint;

	// dirty areas of the texture
	std::deque<CleanUpRequest> _cleanUpRequests;

	// a mutex to protect access to _cleanUpRequests
	boost::mutex _cleanUpRequestsMutex;

	// an area who's content we should keep in memory for incremental drawing 
	// operations
	util::rect<int> _workingArea;

	// the four parts of the working area and their offsets in the texture
	util::rect<int>  _workingAreaParts[4];
	util::point<int> _workingAreaOffsets[4];

	// the up-to-four buffers for the working area
	gui::Buffer* _workingBuffers[4];

	// the clean-up area a thread is currently working on
	util::rect<int> _currentCleanUpArea;
};

#endif // YANTA_PREFETCH_TEXTURE_H__

