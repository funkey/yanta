#include <cmath>

#include <gui/OpenGl.h>
#include <util/Logger.h>
#include "CanvasPainter.h"

logger::LogChannel canvaspainterlog("canvaspainterlog", "[CanvasPainter] ");

CanvasPainter::CanvasPainter() :
	_canvasTexture(0),
	_canvasBufferX(0),
	_canvasBufferY(0),
	_bufferWidth(100),
	_bufferHeight(100),
	_prefetchLeft(100),
	_prefetchRight(100),
	_prefetchTop(100),
	_prefetchBottom(100),
	_state(Moving),
	_drawnUntilStroke(0),
	_drawnUntilStrokePoint(0) {}

void
CanvasPainter::draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	if (!_strokes) {

		LOG_DEBUG(canvaspainterlog) << "no strokes to paint (yet)" << std::endl;
		return;
	}

	//LOG_ALL(canvaspainterlog) << "redrawing in " << roi << " with resolution " << resolution << std::endl;

	// Compute the closest canvas pixel roi to the desired roi in device units.  
	// Later, we are only using our discrete pixel roi for drawing the texture 
	// and displaying the texture. Just the transformation of canvas pixels to 
	// device units will depend on the roi the texture was aligned with 
	// initially (with a call to initiateFullRedraw()).
	util::rect<int> canvasPixelRoi = snapToPixelGrid(roi, resolution);

	//LOG_ALL(canvaspainterlog) << "snapped to pixel grid gives " << canvasPixelRoi << std::endl;

	gui::OpenGl::Guard guard;

	int textureWidth  = canvasPixelRoi.width()  + _prefetchLeft + _prefetchRight;
	int textureHeight = canvasPixelRoi.height() + _prefetchTop  + _prefetchBottom;

	//LOG_ALL(canvaspainterlog) << "with pre-fetch buffers, canvas texture has to be of size " << textureWidth << "x" << textureHeight << std::endl;

	if (prepareTexture(textureWidth, textureHeight))
		initiateFullRedraw(canvasPixelRoi, roi, resolution);

	// canvasPixelRoi did not change
	if (canvasPixelRoi == _previousCanvasPixelRoi) {

		LOG_ALL(canvaspainterlog) << "canvas pixel roi did not change -- I just quickly update the strokes" << std::endl;

		// if we were moving previously, the texture buffer got state -- need to 
		// redraw everythin
		if (_state == Moving) {

			initiateFullRedraw(canvasPixelRoi, roi, resolution);
			_state = IncrementalDrawing;
		}

		updateStrokes(*_strokes, canvasPixelRoi);

	// size of canvas pixel roi changed -- texture needs to be redrawn 
	// completely
	} else if (sizeChanged(resolution, _previousResolution)) {

		LOG_ALL(canvaspainterlog) << "canvas pixel roi changed in size" << std::endl;

		_state = Moving;

		initiateFullRedraw(canvasPixelRoi, roi, resolution);
		updateStrokes(*_strokes, canvasPixelRoi);

	// canvas pixel roi was moved
	} else {

		LOG_ALL(canvaspainterlog) << "canvas pixel roi was moved by " << (canvasPixelRoi.upperLeft() - _previousCanvasPixelRoi.upperLeft()) << std::endl;

		_state = Moving;

		// show a different part of the canvas texture
		shiftTexture(canvasPixelRoi.upperLeft() - _previousCanvasPixelRoi.upperLeft());
	}

	// TODO: do this in a separate thread
	cleanDirtyAreas();

	drawTexture(canvasPixelRoi);

	_previousCanvasPixelRoi = canvasPixelRoi;
	_previousResolution     = resolution;
}

util::rect<int>
CanvasPainter::snapToPixelGrid(const util::rect<double>& roi, const util::point<double>& resolution) {

	// estimate width of roi in pixels
	unsigned int width  = (unsigned int)round(roi.width()*resolution.x);
	unsigned int height = (unsigned int)round(roi.height()*resolution.y);

	// estimate pixel coordinates of upper left corner of roi
	int x = (int)round(roi.minX*resolution.x);
	int y = (int)round(roi.minY*resolution.y);

	return util::rect<int>(x, y, x + width, y + height);
}

bool
CanvasPainter::prepareTexture(int textureWidth, int textureHeight) {

	if (_canvasTexture != 0 && (_canvasTexture->width() != textureWidth || _canvasTexture->height() != textureHeight)) {

		//LOG_ALL(canvaspainterlog) << "canvas texture is of different size, create a new one" << std::endl;

		delete _canvasTexture;
		delete _canvasBufferX;
		delete _canvasBufferY;
		_canvasTexture = 0;
		_canvasBufferX = 0;
		_canvasBufferY = 0;
	}

	if (_canvasTexture == 0) {

		GLenum format = gui::detail::pixel_format_traits<gui::cairo_pixel_t>::gl_format;
		GLenum type   = gui::detail::pixel_format_traits<gui::cairo_pixel_t>::gl_type;

		_canvasTexture = new gui::Texture(textureWidth, textureHeight, GL_RGBA);
		_canvasBufferX = new gui::Buffer(_bufferWidth, textureHeight, format, type);
		_canvasBufferY = new gui::Buffer(textureWidth, _bufferHeight, format, type);

		return true;
	}

	return false;
}

void
CanvasPainter::refresh() {

	LOG_ALL(canvaspainterlog) << "refresh requested" << std::endl;
	resetIncrementalDrawing();
}

void
CanvasPainter::updateStrokes(const Strokes& strokes, const util::rect<int>& roi) {

	// is it necessary to draw something?
	if (_drawnUntilStroke > 1 && _drawnUntilStroke == strokes.size() && _drawnUntilStrokePoint == strokes[_drawnUntilStroke - 1].size())
		return;

	// map the texture memory
	gui::cairo_pixel_t* data = _canvasTexture->map<gui::cairo_pixel_t>();

	drawStrokes(
			data,
			strokes,
			_textureArea,
			_splitCenter,
			roi,
			true);

	// unmap the texture memory
	_canvasTexture->unmap<gui::cairo_pixel_t>();
}

void
CanvasPainter::drawStrokes(
		gui::cairo_pixel_t*     data,
		const Strokes&          strokes,
		const util::rect<int>&  dataArea,
		const util::point<int>& splitCenter,
		const util::rect<int>&  /*roi*/,
		bool                    incremental) {

	//LOG_ALL(canvaspainterlog) << "drawing strokes with roi " << roi << std::endl;

	unsigned int width  = dataArea.width();
	unsigned int height = dataArea.height();

	// wrap the buffer in a cairo surface
	_surface =
			cairo_image_surface_create_for_data(
					(unsigned char*)data,
					CAIRO_FORMAT_ARGB32,
					width,
					height,
					cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width));

	// create a context for the surface
	_context = cairo_create(_surface);

	// Now, we have a surface of widthxheight, with (0,0) being the upper left 
	// corner and (width-1,height-1) the lower right. Translate operations, such 
	// that the upper left is dataArea.upperLeft() and lower right is 
	// dataArea.lowerRight().

	// translate dataArea.upperLeft() to (0,0)
	util::point<int> translate = -dataArea.upperLeft();
	cairo_translate(_context, translate.x, translate.y);

	//LOG_ALL(canvaspainterlog) << "cairo translate (texture to canvas pixels): " << translate << std::endl;

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

				clip.minX = splitCenter.x;
				clip.minY = splitCenter.y;
				clip.maxX = dataArea.maxX;
				clip.maxY = dataArea.maxY;

				translate = dataArea.upperLeft() - splitCenter;
				break;

			// upper right
			case 1:

				clip.minX = dataArea.minX;
				clip.minY = splitCenter.y;
				clip.maxX = splitCenter.x;
				clip.maxY = dataArea.maxY;

				translate = dataArea.upperRight() - splitCenter;
				break;

			// lower left
			case 2:

				clip.minX = splitCenter.x;
				clip.minY = dataArea.minY;
				clip.maxX = dataArea.maxX;
				clip.maxY = splitCenter.y;

				translate = dataArea.lowerLeft() - splitCenter;
				break;

			// lower right
			case 3:

				clip.minX = dataArea.minX;
				clip.minY = dataArea.minY;
				clip.maxX = splitCenter.x;
				clip.maxY = splitCenter.y;

				translate = dataArea.lowerRight() - splitCenter;
				break;
		}

		cairo_save(_context);

		// move operations to correct texture part
		cairo_translate(_context, translate.x, translate.y);

		//LOG_ALL(canvaspainterlog) << "split center translate for part " << part << ": " << translate << std::endl;

		// clip, such that we don't draw outside of our responsibility
		// TODO: check if clip is not one pixel to big
		cairo_rectangle(_context, clip.minX, clip.minY, clip.width(), clip.height());
		cairo_clip(_context);

		//LOG_ALL(canvaspainterlog) << "clip for part " << part << ": " << clip << std::endl;

		// Currently, we are still in canvas pixel coordinates. Transform them 
		// to device units.

		// translate should be performed...
		cairo_translate(_context, _pixelOffset.x, _pixelOffset.y);
		// ...after the scaling
		cairo_scale(_context, _pixelsPerDeviceUnit.x, _pixelsPerDeviceUnit.y);

		//LOG_ALL(canvaspainterlog) << "cairo translate (device units to canvas pixels): " << _pixelOffset         << std::endl;
		//LOG_ALL(canvaspainterlog) << "cairo scale     (device units to canvas pixels): " << _pixelsPerDeviceUnit << std::endl;

		// transfrom the clip accordingly (here we have to use the inverse 
		// mapping)
		util::rect<double> deviceRoi = clip*_deviceUnitsPerPixel + _deviceOffset;

		//LOG_ALL(canvaspainterlog) << "clip in device units: " << deviceRoi << std::endl;

		// draw the (new) strokes in the current part
		if (incremental)
			_cairoPainter.draw(_context, deviceRoi, _drawnUntilStroke, _drawnUntilStrokePoint);
		else
			_cairoPainter.draw(_context, deviceRoi);

		cairo_restore(_context);
	}

	if (incremental) {

		// remember what we drew already
		_drawnUntilStroke = std::max(0, static_cast<int>(strokes.size()) - 1);

		if (strokes.size() > 0)
			_drawnUntilStrokePoint = std::max(0, static_cast<int>(strokes[_drawnUntilStroke].size()) - 1);
		else
			_drawnUntilStrokePoint = 0;
	}

	// cleanup
	cairo_destroy(_context);
	cairo_surface_destroy(_surface);
}

void
CanvasPainter::drawTexture(const util::rect<int>& roi) {

	// draw the texture
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	_canvasTexture->bind();

	// The texture's ROI is in general split into four parts that we have to 
	// draw individually.

	// the position of the rectangle to draw
	util::rect<int>  position;

	// the canvas pixel translation for the texCoords
	util::point<int> translate;

	// the texCoords
	util::rect<double> texCoords;

	for (int part = 0; part < 4; part++) {

		//LOG_ALL(canvaspainterlog) << "drawing texture part " << part << std::endl;

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

		//LOG_ALL(canvaspainterlog) << "\ttexture coordinates are " << texCoords << std::endl;
		//LOG_ALL(canvaspainterlog) << "\tposition is " << position << std::endl;

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
CanvasPainter::shiftTexture(const util::point<int>& shift) {

	// shift the are represented by the texture
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

	//LOG_ALL(canvaspainterlog) << "texture area is now " << _textureArea << ", split center is at " << _splitCenter << std::endl;
}

void
CanvasPainter::markDirty(const util::rect<int>& area) {

	//LOG_ALL(canvaspainterlog) << "area  " << area << " is dirty, now" << std::endl;

	_dirtyAreas.push_back(area);
}

void
CanvasPainter::cleanDirtyAreas() {

	int textureWidth  = _canvasTexture->width();
	//int textureHeight = _canvasTexture->height();

	for (unsigned int i = 0; i < _dirtyAreas.size(); i++) {

		util::rect<int>& area = _dirtyAreas[i];

		//LOG_ALL(canvaspainterlog) << "cleaning area " << area << std::endl;

		if (area.width() < area.height()) {

			// should be handled by x-buffer
			//LOG_ALL(canvaspainterlog) << "cleaning it with x-buffer" << std::endl;

			// process in stripes
			while (area.width() > 0) {

				// align buffer area with left side of dirty area
				util::rect<int> bufferArea;
				bufferArea.minX = area.minX;
				bufferArea.maxX = area.minX + _bufferWidth;
				bufferArea.minY = _textureArea.minY;
				bufferArea.maxY = _textureArea.maxY;

				// move it left, if it leaves the texture area on the right
				if (bufferArea.maxX >= _textureArea.maxX)
					bufferArea += util::point<int>(_textureArea.maxX - bufferArea.maxX, 0);

				//LOG_ALL(canvaspainterlog) << "will clean it with stripe at " << bufferArea << std::endl;

				// map buffer
				gui::cairo_pixel_t* data = _canvasBufferX->map<gui::cairo_pixel_t>();

				util::point<int> splitCenter = _splitCenter;
				if (splitCenter.x < bufferArea.minX)
					splitCenter.x = bufferArea.minX;
				if (splitCenter.x > bufferArea.maxX)
					splitCenter.x = bufferArea.maxX;

				drawStrokes(
						data,
						*_strokes,
						bufferArea,
						splitCenter,
						bufferArea, // we have to draw everywhere, since we don't know the previous contents
						false);

				// unmap buffer
				_canvasBufferX->unmap();

				// update texture
				int offset = bufferArea.minX - _splitCenter.x;
				if (offset < 0)
					offset += _textureArea.width();

				//unsigned int pixelOffset = (offset*textureWidth)/_textureArea.width();
				//LOG_ALL(canvaspainterlog) << "pixel offset for this stripe is " << pixelOffset << std::endl;

				if (offset > textureWidth - (int)_bufferWidth) {

					LOG_ERROR(canvaspainterlog) << "pixel offset for x-buffer stripe is too big: " << offset << std::endl;
					offset = textureWidth - _bufferWidth;
				}

				_canvasTexture->loadData(*_canvasBufferX, offset, 0);

				// remove updated part from dirty area
				area.minX += _bufferWidth;
			}
		}
	}

	_dirtyAreas.clear();
}

void
CanvasPainter::resetIncrementalDrawing() {

	// draw all strokes, the next time
	_drawnUntilStroke = 0;
	_drawnUntilStrokePoint = 0;
}

void
CanvasPainter::initiateFullRedraw(
		const util::rect<int>&     canvasPixelRoi,
		const util::rect<double>&  deviceRoi,
		const util::point<double>& resolution) {

	resetIncrementalDrawing();

	// recompute area represented by canvas texture and buffers
	_textureArea.minX = canvasPixelRoi.minX - _prefetchLeft;
	_textureArea.minY = canvasPixelRoi.minY - _prefetchTop;
	_textureArea.maxX = canvasPixelRoi.maxX + _prefetchRight;
	_textureArea.maxY = canvasPixelRoi.maxY + _prefetchBottom;

	// center the current roi in the texture
	_splitCenter = _textureArea.upperLeft();

	// remember the mapping from texture coordinates to device units
	_deviceUnitsPerPixel = util::point<double>(1.0/resolution.x, 1.0/resolution.y);
	_deviceOffset        = deviceRoi.upperLeft() - _deviceUnitsPerPixel*canvasPixelRoi.upperLeft();
	_pixelsPerDeviceUnit = resolution;
	_pixelOffset         = util::point<double>(-_deviceOffset.x*resolution.x, -_deviceOffset.y*resolution.y);
}

bool
CanvasPainter::sizeChanged(const util::point<double>& resolution, const util::point<double>& previousResolution) {

	return (resolution != previousResolution);
}

