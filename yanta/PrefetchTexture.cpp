#include <SkBitmap.h>
#include <SkCanvas.h>
#include <gui/Skia.h>
#include <gui/OpenGl.h>
#include <util/Logger.h>
#include "PrefetchTexture.h"

logger::LogChannel prefetchtexturelog("prefetchtexturelog", "[PrefetchTexture] ");

PrefetchTexture::PrefetchTexture(
		const util::rect<int>& area,
		unsigned int prefetchLeft,
		unsigned int prefetchRight,
		unsigned int prefetchTop,
		unsigned int prefetchBottom) :
	_prefetchLeft(prefetchLeft),
	_prefetchRight(prefetchRight),
	_prefetchTop(prefetchTop),
	_prefetchBottom(prefetchBottom),
	_texture(0),
	_bufferWidth(std::min(prefetchLeft, prefetchRight)),
	_bufferHeight(std::min(prefetchTop, prefetchBottom)),
	_reloadBufferX(0),
	_reloadBufferY(0),
	_workingArea(0, 0, 0, 0),
	_currentCleanUpArea(0, 0, 0, 0) {

	for (int i = 0; i < 4; i++)
		_workingBuffers[i] = 0;

	reset(area);
	prepare();
}

PrefetchTexture::~PrefetchTexture() {

	gui::OpenGl::Guard guard;

	for (int i = 0; i < 4; i++)
		deleteBuffer(&_workingBuffers[i]);
	deleteBuffer(&_reloadBufferX);
	deleteBuffer(&_reloadBufferY);

	//boost::mutex::scoped_lock lock(_textureMutex);

	if (_texture)
		delete _texture;
}

bool
PrefetchTexture::prepare() {

	int width  = _textureArea.width();
	int height = _textureArea.height();

	if (_texture != 0 && (_texture->width() != width || _texture->height() != height)) {

		LOG_ALL(prefetchtexturelog) << "texture is of different size, create a new one" << std::endl;

		delete _texture;
		delete _reloadBufferX;
		delete _reloadBufferY;
		_texture = 0;
		_reloadBufferX = 0;
		_reloadBufferY = 0;
	}

	if (_texture == 0) {

		GLenum format = gui::detail::pixel_format_traits<gui::skia_pixel_t>::gl_format;
		GLenum type   = gui::detail::pixel_format_traits<gui::skia_pixel_t>::gl_type;

		_texture = new gui::Texture(width, height, GL_RGBA);
		_reloadBufferX = new gui::Buffer(_bufferWidth, height, format, type);
		_reloadBufferY = new gui::Buffer(width, _bufferHeight, format, type);

		return true;
	}

	return false;
}


void
PrefetchTexture::fill(
		const util::rect<int>& subarea,
		SkiaCanvasPainter& painter) {

	// get the up-to-four parts of the subarea
	util::rect<int>  parts[4];
	util::point<int> offsets[4];

	split(subarea, parts, offsets);

	for (int i = 0; i < 4; i++) {

		if (parts[i].area() <= 0)
			continue;

		LOG_ALL(prefetchtexturelog) << "filling part " << i << " " << parts[i] << std::endl;

		unsigned int width  = parts[i].width();
		unsigned int height = parts[i].height();

		// if we are about to fill the region specified by setWorkingArea(), we 
		// reuse the already allocated buffers
		if (_workingArea.contains(subarea)) {

			LOG_ALL(prefetchtexturelog) << "specified subarea is part of working area -- reuse working buffers" << std::endl;

			// fill the _workingBuffers[i], representing area 
			// _workingAreaParts[i] only in the subarea parts[i]
			fillBuffer(*_workingBuffers[i], _workingAreaParts[i], painter, parts[i]);

			_texture->loadData(*_workingBuffers[i], _workingAreaOffsets[i].x, _workingAreaOffsets[i].y);

		} else {

			gui::Buffer* buffer = 0;

			LOG_ALL(prefetchtexturelog) << "creating new buffers for the subarea update" << std::endl;
			createBuffer(width, height, &buffer);

			fillBuffer(*buffer, parts[i], painter, parts[i]);

			_texture->loadData(*buffer, offsets[i].x, offsets[i].y);

			LOG_ALL(prefetchtexturelog) << "put reload buffer at " << offsets[i] << std::endl;

			deleteBuffer(&buffer);
		}
	}
}

void
PrefetchTexture::fillBuffer(
		gui::Buffer&           buffer,
		const util::rect<int>& bufferArea,
		SkiaCanvasPainter&     painter,
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

	LOG_ALL(prefetchtexturelog) << "skia translate (texture to canvas pixels): " << translate << std::endl;

	painter.draw(canvas, roi);

	// unmap the buffer
	buffer.unmap();
}

void
PrefetchTexture::setWorkingArea(const util::rect<int>& subarea) {

	// get the up-to-four parts of the subarea
	util::rect<int>  parts[4];
	util::point<int> offsets[4];

	split(subarea, parts, offsets);

	gui::OpenGl::Guard guard;

	for (int i = 0; i < 4; i++) {

		if (parts[i].area() <= 0) {

			deleteBuffer(&_workingBuffers[i]);
			continue;
		}

		LOG_ALL(prefetchtexturelog) << "creating working buffer " << i << " for " << parts[i] << std::endl;

		unsigned int width  = parts[i].width();
		unsigned int height = parts[i].height();

		createBuffer(width, height, &_workingBuffers[i]);

		_workingAreaParts[i]   = parts[i];
		_workingAreaOffsets[i] = offsets[i];
	}

	_workingArea = subarea;
}

void
PrefetchTexture::render(const util::rect<int>& roi) {

	// draw the texture
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//boost::mutex::scoped_lock lock(_textureMutex);

	_texture->bind();

	// The texture's ROI is in general split into four parts that we have to 
	// draw individually. On the fly, make sure that the requested region is not 
	// larger than the one that is covered by this texture by intersecting it 
	// with the texture area.

	// the texCoords
	util::rect<double> texCoords;

	util::rect<int>  parts[4];
	util::point<int> offsets[4];

	split(roi.intersection(_textureArea), parts, offsets);

	for (int i = 0; i < 4; i++) {

		if (parts[i].area() <= 0)
			continue;

		LOG_ALL(prefetchtexturelog) << "drawing texture part " << i << std::endl;

		// offset is in canvas pixels, use it with parts[i] to determine the 
		// texCoords
		texCoords = parts[i] - parts[i].upperLeft() + offsets[i];
		texCoords.minX /= _textureArea.width();
		texCoords.maxX /= _textureArea.width();
		texCoords.minY /= _textureArea.height();
		texCoords.maxY /= _textureArea.height();

		LOG_ALL(prefetchtexturelog) << "\ttexture coordinates are " << texCoords << std::endl;
		LOG_ALL(prefetchtexturelog) << "\tpart is " << parts[i] << std::endl;

		// DEBUG
		//glDisable(GL_TEXTURE_2D);
		//glColor3f(1.0/(i+1), 1.0/(4-i+1), 1.0);
		//glBegin(GL_QUADS);
		//glVertex2d(parts[i].minX, parts[i].minY);
		//glVertex2d(parts[i].maxX, parts[i].minY);
		//glVertex2d(parts[i].maxX, parts[i].maxY);
		//glVertex2d(parts[i].minX, parts[i].maxY);
		//glEnd();
		//glEnable(GL_TEXTURE_2D);
		// END DEBUG

		glColor3f(1.0f, 1.0f, 1.0f);
		glBegin(GL_QUADS);
		glTexCoord2d(texCoords.minX, texCoords.minY); glVertex2d(parts[i].minX, parts[i].minY);
		glTexCoord2d(texCoords.maxX, texCoords.minY); glVertex2d(parts[i].maxX, parts[i].minY);
		glTexCoord2d(texCoords.maxX, texCoords.maxY); glVertex2d(parts[i].maxX, parts[i].maxY);
		glTexCoord2d(texCoords.minX, texCoords.maxY); glVertex2d(parts[i].minX, parts[i].maxY);
		glEnd();
	}

	// DEBUG
	//util::rect<double> debug = roi;
	//debug -= roi.upperLeft();
	//debug *= 0.25;
	//debug += roi.upperLeft();
	//glDisable(GL_TEXTURE_2D);
	//glColor3f(1.0, 1.0, 1.0);
	//glBegin(GL_QUADS);
	//glVertex2d(debug.minX, debug.minY);
	//glVertex2d(debug.maxX, debug.minY);
	//glVertex2d(debug.maxX, debug.maxY);
	//glVertex2d(debug.minX, debug.maxY);
	//glEnd();
	//glEnable(GL_TEXTURE_2D);
	//glBegin(GL_QUADS);
	//glTexCoord2d(0, 0); glVertex2d(debug.minX, debug.minY);
	//glTexCoord2d(1, 0); glVertex2d(debug.maxX, debug.minY);
	//glTexCoord2d(1, 1); glVertex2d(debug.maxX, debug.maxY);
	//glTexCoord2d(0, 1); glVertex2d(debug.minX, debug.maxY);
	//glEnd();
	// END DEBUG

	glDisable(GL_BLEND);
}

void
PrefetchTexture::shift(const util::point<int>& shift) {

	LOG_ALL(prefetchtexturelog) << "shifting texture content by " << shift << std::endl;

	// shift the area represented by the texture
	_textureArea -= shift;

	// make sure the split point is within the area
	while (_splitPoint.x < _textureArea.minX)
		_splitPoint.x += _textureArea.width();
	while (_splitPoint.y < _textureArea.minY)
		_splitPoint.y += _textureArea.height();
	while (_splitPoint.x >= _textureArea.maxX)
		_splitPoint.x -= _textureArea.width();
	while (_splitPoint.y >= _textureArea.maxY)
		_splitPoint.y -= _textureArea.height();

	LOG_ALL(prefetchtexturelog) << "texture area is now " << _textureArea << std::endl;
	LOG_ALL(prefetchtexturelog) << "split point is now at " << _splitPoint << std::endl;

	// the edge in front of the shift is dirty, now

	util::rect<int> dirtyX;
	util::rect<int> dirtyY;

	dirtyX.minY = _textureArea.minY;
	dirtyX.maxY = _textureArea.maxY;
	dirtyX.minX = (shift.x < 0 ? _textureArea.maxX + shift.x : _textureArea.minX);
	dirtyX.maxX = (shift.x < 0 ? _textureArea.maxX           : _textureArea.minX + shift.x);

	dirtyY.minY = (shift.y < 0 ? _textureArea.maxY + shift.y : _textureArea.minY);
	dirtyY.maxY = (shift.y < 0 ? _textureArea.maxY           : _textureArea.minY + shift.y);
	dirtyY.minX = (shift.x < 0 ? _textureArea.minX : dirtyX.maxX);
	dirtyY.maxX = (shift.x < 0 ? dirtyX.minX       : _textureArea.maxX);

	markDirty(dirtyX);
	markDirty(dirtyY);
}

void
PrefetchTexture::markDirty(const util::rect<int>& area) {

	LOG_ALL(prefetchtexturelog) << "area  " << area << " is dirty, now" << std::endl;

	util::rect<int>  parts[4];
	util::point<int> offsets[4];

	split(area, parts, offsets);

	for (int i = 0; i < 4; i++) {

		if (parts[i].area() <= 0)
			continue;

		CleanUpRequest request(parts[i], offsets[i]);

		boost::mutex::scoped_lock lock(_cleanUpRequestsMutex);

		_cleanUpRequests.push_back(request);
	}
}

void
PrefetchTexture::reset(const util::rect<int>& area) {

	// recompute area represented by canvas texture and buffers
	_textureArea.minX = area.minX - _prefetchLeft;
	_textureArea.minY = area.minY - _prefetchTop;
	_textureArea.maxX = area.maxX + _prefetchRight;
	_textureArea.maxY = area.maxY + _prefetchBottom;

	// center the current roi in the texture
	_splitPoint = _textureArea.upperLeft();

	{
		boost::mutex::scoped_lock lock(_cleanUpRequestsMutex);

		// dismiss all pending clean-up requests
		_cleanUpRequests.clear();
	}

	// mark everything as dirty
	markDirty(_textureArea);
}

void
PrefetchTexture::split(const util::rect<int>& subarea, util::rect<int>* parts, util::point<int>* offsets) {

	// upper left
	parts[0].minX = std::max(_splitPoint.x, subarea.minX);
	parts[0].minY = std::max(_splitPoint.y, subarea.minY);
	parts[0].maxX = subarea.maxX;
	parts[0].maxY = subarea.maxY;

	// upper right
	parts[1].minX = subarea.minX;
	parts[1].minY = std::max(_splitPoint.y, subarea.minY);
	parts[1].maxX = std::min(_splitPoint.x, subarea.maxX);
	parts[1].maxY = subarea.maxY;

	// lower left
	parts[2].minX = std::max(_splitPoint.x, subarea.minX);
	parts[2].minY = subarea.minY;
	parts[2].maxX = subarea.maxX;
	parts[2].maxY = std::min(_splitPoint.y, subarea.maxY);

	// lower right
	parts[3].minX = subarea.minX;
	parts[3].minY = subarea.minY;
	parts[3].maxX = std::min(_splitPoint.x, subarea.maxX);
	parts[3].maxY = std::min(_splitPoint.y, subarea.maxY);

	// the offsets are the positions of the upper left pixel of the parts in the 
	// texture
	for (int i = 0; i < 4; i++) {

		if (parts[i].width() <= 0 || parts[i].height() <= 0) {

			parts[i] = util::rect<int>(0, 0, 0, 0);
			offsets[i] = util::point<int>(0, 0);
			continue;
		}

		offsets[i] = parts[i].upperLeft() - _splitPoint;

		while (offsets[i].x <  0) offsets[i].x += _textureArea.width();
		while (offsets[i].y <  0) offsets[i].y += _textureArea.height();
		while (offsets[i].x >= _textureArea.width())  offsets[i].x -= _textureArea.width();
		while (offsets[i].y >= _textureArea.height()) offsets[i].y -= _textureArea.height();
	}
}

void
PrefetchTexture::createBuffer(unsigned int width, unsigned int height, gui::Buffer** buffer) {

	deleteBuffer(buffer);

	GLenum format = gui::detail::pixel_format_traits<gui::cairo_pixel_t>::gl_format;
	GLenum type   = gui::detail::pixel_format_traits<gui::cairo_pixel_t>::gl_type;

	*buffer = new gui::Buffer(width, height, format, type);
}

void
PrefetchTexture::deleteBuffer(gui::Buffer** buffer) {

	if (*buffer)
		delete *buffer;

	*buffer = 0;
}

bool
PrefetchTexture::getNextCleanUpRequest(CleanUpRequest& request) {

	boost::mutex::scoped_lock lock(_cleanUpRequestsMutex);

	if (_cleanUpRequests.size() == 0)
		return false;

	request = _cleanUpRequests.front();
	_cleanUpRequests.pop_front();

	_currentCleanUpArea = request.area;

	return true;
}

void
PrefetchTexture::cleanUp(SkiaCanvasPainter& painter, unsigned int maxNumRequests) {

	gui::OpenGl::Guard guard;

	unsigned int numRequests = std::min(static_cast<unsigned int>(_cleanUpRequests.size()), maxNumRequests);

	LOG_DEBUG(prefetchtexturelog) << "processing " << numRequests << " cleanup requests" << std::endl;

	gui::Buffer* buffer = 0;
	CleanUpRequest request;

	for (unsigned int i = 0; i < numRequests; i++) {

		// the number of requests might have decreased while we were working on 
		// them, in this case abort and come back later
		if (!getNextCleanUpRequest(request))
			return;

		createBuffer(request.area.width(), request.area.height(), &buffer);

		fillBuffer(*buffer, request.area, painter, request.area);

		{
			//boost::mutex::scoped_lock lock(_textureMutex);

			// update texture with buffer content
			_texture->loadData(*buffer, request.textureOffset.x, request.textureOffset.y);
		}

		deleteBuffer(&buffer);

		_currentCleanUpArea = util::rect<int>(0, 0, 0, 0);
	}

	LOG_DEBUG(prefetchtexturelog) << "done cleaning up" << std::endl;
}

bool
PrefetchTexture::isDirty(const util::rect<int>& roi) {

	// is it in _cleanUpRequests?
	{
		boost::mutex::scoped_lock lock(_cleanUpRequestsMutex);

		for (std::deque<CleanUpRequest>::const_iterator i = _cleanUpRequests.begin(); i != _cleanUpRequests.end(); i++)
			if (i->area.intersects(roi)) {

				LOG_ALL(prefetchtexturelog) << "roi " << roi << " is dirty because of request in " << i->area << std::endl;
				return true;
			}
	}

	// is it currently worked on?
	if (_currentCleanUpArea.intersects(roi)) {

		LOG_ALL(prefetchtexturelog) << "roi " << roi << " is dirty because of current work in " << _currentCleanUpArea << std::endl;
		return true;
	}

	return false;
}
