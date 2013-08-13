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
		_texture = 0;
	}

	if (_texture == 0) {

		_texture = new gui::Texture(width, height, GL_RGBA);

		return true;
	}

	return false;
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

		// offset is in document pixels, use it with parts[i] to determine the 
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

		_cleanUpRequests.push_back(request);
	}
}

void
PrefetchTexture::reset(const util::rect<int>& area) {

	// recompute area represented by document texture and buffers
	_textureArea.minX = area.minX - _prefetchLeft;
	_textureArea.minY = area.minY - _prefetchTop;
	_textureArea.maxX = area.maxX + _prefetchRight;
	_textureArea.maxY = area.maxY + _prefetchBottom;

	// center the current roi in the texture
	_splitPoint = _textureArea.upperLeft();

	{
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
PrefetchTexture::createBuffer(unsigned int width, unsigned int height, gui::skia_pixel_t** buffer) {

	deleteBuffer(buffer);

	*buffer = new gui::skia_pixel_t[width*height];
}

void
PrefetchTexture::deleteBuffer(gui::skia_pixel_t** buffer) {

	if (*buffer)
		delete[] *buffer;

	*buffer = 0;
}

bool
PrefetchTexture::getNextCleanUpRequest(CleanUpRequest& request) {

	if (_cleanUpRequests.size() == 0)
		return false;

	request = _cleanUpRequests.front();
	_cleanUpRequests.pop_front();

	_currentCleanUpArea = request.area;

	return true;
}

bool
PrefetchTexture::isDirty(const util::rect<int>& roi) {

	// is it in _cleanUpRequests?
	for (std::deque<CleanUpRequest>::const_iterator i = _cleanUpRequests.begin(); i != _cleanUpRequests.end(); i++)
		if (i->area.intersects(roi)) {

			LOG_ALL(prefetchtexturelog) << "roi " << roi << " is dirty because of request in " << i->area << std::endl;
			return true;
		}

	// is it currently worked on?
	if (_currentCleanUpArea.intersects(roi)) {

		LOG_ALL(prefetchtexturelog) << "roi " << roi << " is dirty because of current work in " << _currentCleanUpArea << std::endl;
		return true;
	}

	return false;
}
