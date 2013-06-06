#include <gui/Cairo.h>
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
	_bufferWidth(std::min(prefetchLeft, prefetchRight)),
	_bufferHeight(std::min(prefetchTop, prefetchBottom)),
	_texture(0),
	_reloadBufferX(0),
	_reloadBufferY(0) {

	reset(area);
	prepare();
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

		GLenum format = gui::detail::pixel_format_traits<gui::cairo_pixel_t>::gl_format;
		GLenum type   = gui::detail::pixel_format_traits<gui::cairo_pixel_t>::gl_type;

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
		CairoCanvasPainter& painter) {

	// TODO:
	//
	// • create buffer(s)
	// • map them
	// • call painter.draw() on them
	// • unmap them
	// • update texture

	// TODO workaround for now: slow but exact version
	gui::cairo_pixel_t*    data     = _texture->map<gui::cairo_pixel_t>();
	const util::rect<int>& dataArea = _textureArea;

		//gui::cairo_pixel_t*     data,
		//const Strokes&          strokes,
		//const util::rect<int>&  dataArea,
		//const util::point<int>& splitCenter,
		//const util::rect<int>&  [>roi<],
		//bool                    incremental) {

	LOG_ALL(prefetchtexturelog) << "filling subarea " << subarea << std::endl;

	unsigned int width  = dataArea.width();
	unsigned int height = dataArea.height();

	// wrap the buffer in a cairo surface
	cairo_surface_t* surface =
			cairo_image_surface_create_for_data(
					(unsigned char*)data,
					CAIRO_FORMAT_ARGB32,
					width,
					height,
					cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width));

	// create a context for the surface
	cairo_t* context = cairo_create(surface);

	// Now, we have a surface of widthxheight, with (0,0) being the upper left 
	// corner and (width-1,height-1) the lower right. Translate operations, such 
	// that the upper left is dataArea.upperLeft() and lower right is 
	// dataArea.lowerRight().

	// translate dataArea.upperLeft() to (0,0)
	util::point<int> translate = -dataArea.upperLeft();
	cairo_translate(context, translate.x, translate.y);

	LOG_ALL(prefetchtexturelog) << "cairo translate (texture to canvas pixels): " << translate << std::endl;

	// Now we could start drawing onto the surface, if the ROI is perfectly 
	// centered.  However, in general, there is an additional shift with 
	// wrap-around that we have to compensate for. This gives four parts of the 
	// data that have to be drawn individually.

	// a rectangle used to clip the cairo operations
	util::rect<int>  clip;

	for (int part = 0; part < 4; part++) {

		switch (part) {

			// upper left
			case 0:

				clip.minX = _splitCenter.x;
				clip.minY = _splitCenter.y;
				clip.maxX = dataArea.maxX;
				clip.maxY = dataArea.maxY;

				translate = dataArea.upperLeft() - _splitCenter;
				break;

			// upper right
			case 1:

				clip.minX = dataArea.minX;
				clip.minY = _splitCenter.y;
				clip.maxX = _splitCenter.x;
				clip.maxY = dataArea.maxY;

				translate = dataArea.upperRight() - _splitCenter;
				break;

			// lower left
			case 2:

				clip.minX = _splitCenter.x;
				clip.minY = dataArea.minY;
				clip.maxX = dataArea.maxX;
				clip.maxY = _splitCenter.y;

				translate = dataArea.lowerLeft() - _splitCenter;
				break;

			// lower right
			case 3:

				clip.minX = dataArea.minX;
				clip.minY = dataArea.minY;
				clip.maxX = _splitCenter.x;
				clip.maxY = _splitCenter.y;

				translate = dataArea.lowerRight() - _splitCenter;
				break;
		}

		cairo_save(context);

		// move operations to correct texture part
		cairo_translate(context, translate.x, translate.y);

		LOG_ALL(prefetchtexturelog) << "split center translate for part " << part << ": " << translate << std::endl;

		LOG_ALL(prefetchtexturelog) << "clip for part " << part << ": " << clip << std::endl;

		// draw the content of the cairo painter
		// TODO: check if clip is not one pixel to big
		painter.draw(context, clip);

		cairo_restore(context);
	}

	// cleanup
	cairo_destroy(context);
	cairo_surface_destroy(surface);

	_texture->unmap<gui::cairo_pixel_t>();
}

// TODO: use glTranslate and glScale instead of passing _device*
void
PrefetchTexture::render(const util::rect<int>& roi, const util::point<double>& _deviceUnitsPerPixel, const util::point<double>& _deviceOffset) {

	// draw the texture
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	_texture->bind();

	// The texture's ROI is in general split into four parts that we have to 
	// draw individually.

	// the position of the rectangle to draw
	util::rect<int> position;

	// the canvas pixel translation for the texCoords
	util::point<int> translate;

	// the texCoords
	util::rect<double> texCoords;

	for (int part = 0; part < 4; part++) {

		LOG_ALL(prefetchtexturelog) << "drawing texture part " << part << std::endl;

		switch (part) {

			// upper left
			case 0:

				position.minX = std::max(_splitCenter.x, roi.minX);
				position.minY = std::max(_splitCenter.y, roi.minY);
				position.maxX = roi.maxX;
				position.maxY = roi.maxY;

				if (position.area() <= 0)
					continue;

				translate = _splitCenter - _textureArea.upperLeft();

				break;

			// upper right
			case 1:

				position.minX = roi.minX;
				position.minY = std::max(_splitCenter.y, roi.minY);
				position.maxX = std::min(_splitCenter.x, roi.maxX);
				position.maxY = roi.maxY;

				if (position.area() <= 0)
					continue;

				translate = _splitCenter - _textureArea.upperRight();

				break;

			// lower left
			case 2:

				position.minX = std::max(_splitCenter.x, roi.minX);
				position.minY = roi.minY;
				position.maxX = roi.maxX;
				position.maxY = std::min(_splitCenter.y, roi.maxY);

				if (position.area() <= 0)
					continue;

				translate = _splitCenter - _textureArea.lowerLeft();

				break;

			// lower right
			case 3:

				position.minX = roi.minX;
				position.minY = roi.minY;
				position.maxX = std::min(_splitCenter.x, roi.maxX);
				position.maxY = std::min(_splitCenter.y, roi.maxY);

				if (position.area() <= 0)
					continue;

				translate = _splitCenter - _textureArea.lowerRight();

				break;
		}

		// translate is in canvas pixels, use it with position to determine the 
		// texCoords
		texCoords = position - translate - _textureArea.upperLeft();
		texCoords.minX /= _textureArea.width();
		texCoords.maxX /= _textureArea.width();
		texCoords.minY /= _textureArea.height();
		texCoords.maxY /= _textureArea.height();

		LOG_ALL(prefetchtexturelog) << "\ttexture coordinates are " << texCoords << std::endl;
		LOG_ALL(prefetchtexturelog) << "\tposition is " << position << std::endl;

		// position is in canvas pixels, map it to device units

		// first scale position to device untis
		util::rect<double> devicePosition = position;
		devicePosition *= _deviceUnitsPerPixel;

		// now, translate it
		devicePosition += _deviceOffset;

		// DEBUG
		glDisable(GL_TEXTURE_2D);
		glColor3f(1.0/(part+1), 1.0/(4-part+1), 1.0);
		glBegin(GL_QUADS);
		glVertex2d(devicePosition.minX, devicePosition.minY);
		glVertex2d(devicePosition.maxX, devicePosition.minY);
		glVertex2d(devicePosition.maxX, devicePosition.maxY);
		glVertex2d(devicePosition.minX, devicePosition.maxY);
		glEnd();
		glEnable(GL_TEXTURE_2D);
		// END DEBUG

		glColor3f(1.0f, 1.0f, 1.0f);
		glBegin(GL_QUADS);
		glTexCoord2d(texCoords.minX, texCoords.minY); glVertex2d(devicePosition.minX, devicePosition.minY);
		glTexCoord2d(texCoords.maxX, texCoords.minY); glVertex2d(devicePosition.maxX, devicePosition.minY);
		glTexCoord2d(texCoords.maxX, texCoords.maxY); glVertex2d(devicePosition.maxX, devicePosition.maxY);
		glTexCoord2d(texCoords.minX, texCoords.maxY); glVertex2d(devicePosition.minX, devicePosition.maxY);
		glEnd();
	}

	// DEBUG
	util::rect<double> debug = 0.25*roi;
	debug += roi.upperLeft() - debug.upperLeft();
	glDisable(GL_TEXTURE_2D);
	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_QUADS);
	glVertex2d(debug.minX, debug.minY);
	glVertex2d(debug.maxX, debug.minY);
	glVertex2d(debug.maxX, debug.maxY);
	glVertex2d(debug.minX, debug.maxY);
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2d(0, 0); glVertex2d(debug.minX, debug.minY);
	glTexCoord2d(1, 0); glVertex2d(debug.maxX, debug.minY);
	glTexCoord2d(1, 1); glVertex2d(debug.maxX, debug.maxY);
	glTexCoord2d(0, 1); glVertex2d(debug.minX, debug.maxY);
	glEnd();
	// END DEBUG

	glDisable(GL_BLEND);
}

void
PrefetchTexture::shift(const util::point<int>& shift) {

	// shift the area represented by the texture
	_textureArea += shift;

	// make sure the split center is within the area
	while (_splitCenter.x < _textureArea.minX)
		_splitCenter.x += _textureArea.width();
	while (_splitCenter.y < _textureArea.minY)
		_splitCenter.y += _textureArea.height();
	while (_splitCenter.x >= _textureArea.maxX)
		_splitCenter.x -= _textureArea.width();
	while (_splitCenter.y >= _textureArea.maxY)
		_splitCenter.y -= _textureArea.height();

	// whatever is in front of us is dirty, now

	util::rect<int> dirtyX;
	util::rect<int> dirtyY;

	dirtyX.minY = _textureArea.minY;
	dirtyX.maxY = _textureArea.maxY;
	dirtyX.minX = (shift.x > 0 ? _textureArea.maxX - shift.x : _textureArea.minX);
	dirtyX.maxX = (shift.x > 0 ? _textureArea.maxX           : _textureArea.minX - shift.x);

	dirtyY.minY = (shift.y > 0 ? _textureArea.maxY - shift.y : _textureArea.minY);
	dirtyY.maxY = (shift.y > 0 ? _textureArea.maxY           : _textureArea.minY - shift.y);
	dirtyY.minX = (shift.x > 0 ? _textureArea.minX : dirtyX.maxX);
	dirtyY.maxX = (shift.x > 0 ? dirtyX.minX       : _textureArea.maxX);

	markDirty(dirtyX);
	markDirty(dirtyY);

	LOG_ALL(prefetchtexturelog) << "texture area is now " << _textureArea << ", split center is at " << _splitCenter << std::endl;
}

void
PrefetchTexture::markDirty(const util::rect<int>& area) {

	LOG_ALL(prefetchtexturelog) << "area  " << area << " is dirty, now" << std::endl;

	_dirtyAreas.push_back(area);
}

void
PrefetchTexture::cleanDirtyAreas(CairoCanvasPainter& painter) {

	for (unsigned int i = 0; i < _dirtyAreas.size(); i++)
		fill(_dirtyAreas[i], painter);

	_dirtyAreas.clear();
}

void
PrefetchTexture::reset(const util::rect<int>& area) {

	// recompute area represented by canvas texture and buffers
	_textureArea.minX = area.minX - _prefetchLeft;
	_textureArea.minY = area.minY - _prefetchTop;
	_textureArea.maxX = area.maxX + _prefetchRight;
	_textureArea.maxY = area.maxY + _prefetchBottom;

	// center the current roi in the texture
	_splitCenter = _textureArea.upperLeft();
}
