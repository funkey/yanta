#ifndef YANTA_PREFETCH_TEXTURE_H__
#define YANTA_PREFETCH_TEXTURE_H__

#include <deque>
#include <boost/thread.hpp>
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
	template <typename Painter>
	void fill(
			const util::rect<int>& subarea,
			Painter&               painter);

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
	template <typename Painter>
	void cleanUp(Painter& painter, unsigned int maxNumRequests = 0);

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
	template <typename Painter>
	void fillBuffer(
			gui::Buffer&           buffer,
			const util::rect<int>& bufferArea,
			Painter&               painter,
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

template <typename Painter>
void
PrefetchTexture::fill(
		const util::rect<int>& subarea,
		Painter&               painter) {

	// get the up-to-four parts of the subarea
	util::rect<int>  parts[4];
	util::point<int> offsets[4];

	split(subarea, parts, offsets);

	for (int i = 0; i < 4; i++) {

		if (parts[i].area() <= 0)
			continue;

		unsigned int width  = parts[i].width();
		unsigned int height = parts[i].height();

		// if we are about to fill the region specified by setWorkingArea(), we 
		// reuse the already allocated buffers
		if (_workingArea.contains(subarea)) {

			// fill the _workingBuffers[i], representing area 
			// _workingAreaParts[i] only in the subarea parts[i]
			fillBuffer(*_workingBuffers[i], _workingAreaParts[i], painter, parts[i]);

			_texture->loadData(*_workingBuffers[i], _workingAreaOffsets[i].x, _workingAreaOffsets[i].y);

		} else {

			gui::Buffer* buffer = 0;

			createBuffer(width, height, &buffer);

			fillBuffer(*buffer, parts[i], painter, parts[i]);

			_texture->loadData(*buffer, offsets[i].x, offsets[i].y);

			deleteBuffer(&buffer);
		}
	}
}

template <typename Painter>
void
PrefetchTexture::fillBuffer(
		gui::Buffer&           buffer,
		const util::rect<int>& bufferArea,
		Painter&               painter,
		const util::rect<int>& roi) {

	unsigned int width  = bufferArea.width();
	unsigned int height = bufferArea.height();

	gui::skia_pixel_t* data = buffer.map<gui::skia_pixel_t>();

	// wrap the buffer in a skia bitmap
	SkBitmap bitmap;
	bitmap.setConfig(SkBitmap::kARGB_8888_Config, width, height);
	bitmap.setPixels(data);

	SkCanvas canvas(bitmap);

	// Now, we have a surface of widthxheight, with (0,0) being the upper left 
	// corner and (width-1,height-1) the lower right. Translate operations, such 
	// that the upper left is bufferArea.upperLeft() and lower right is 
	// bufferArea.lowerRight().

	// translate bufferArea.upperLeft() to (0,0)
	util::point<int> translate = -bufferArea.upperLeft();
	canvas.translate(translate.x, translate.y);

	painter.draw(canvas, roi);

	// unmap the buffer
	buffer.unmap();
}

template <typename Painter>
void
PrefetchTexture::cleanUp(Painter& painter, unsigned int maxNumRequests) {

	gui::OpenGl::Guard guard;

	unsigned int numRequests = _cleanUpRequests.size();
	
	if (maxNumRequests != 0)
		numRequests = std::min(numRequests, maxNumRequests);

	gui::Buffer* buffer = 0;
	CleanUpRequest request;

	for (unsigned int i = 0; i < numRequests; i++) {

		// the number of requests might have decreased while we were working on 
		// them, in this case abort and come back later
		if (!getNextCleanUpRequest(request))
			return;

		createBuffer(request.area.width(), request.area.height(), &buffer);

		fillBuffer(*buffer, request.area, painter, request.area);

		// update texture with buffer content
		_texture->loadData(*buffer, request.textureOffset.x, request.textureOffset.y);

		deleteBuffer(&buffer);

		_currentCleanUpArea = util::rect<int>(0, 0, 0, 0);
	}
}

#endif // YANTA_PREFETCH_TEXTURE_H__

