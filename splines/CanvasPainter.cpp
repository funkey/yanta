#include <cmath>

#include <gui/OpenGl.h>
#include <util/Logger.h>
#include "CanvasPainter.h"

logger::LogChannel canvaspainterlog("canvaspainterlog", "[CanvasPainter] ");

CanvasPainter::CanvasPainter() :
	_canvasTexture(0),
	_canvasBufferX(0),
	_canvasBufferY(0),
	_bufferWidth(50),
	_bufferHeight(100),
	_prefetchLeft(100),
	_prefetchRight(100),
	_prefetchTop(500),
	_prefetchBottom(500),
	_drawnUntilStroke(0),
	_drawnUntilStrokePoint(0) {}

void
CanvasPainter::draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	LOG_ALL(canvaspainterlog) << "redrawing in " << roi << " with resolution " << resolution << std::endl;

	if (!_strokes) {

		LOG_DEBUG(canvaspainterlog) << "no strokes to paint (yet)" << std::endl;
		return;
	}

	gui::OpenGl::Guard guard;

	// estimate screen size in pixels
	_screenWidth  = (unsigned int)round(roi.width()*resolution.x);
	_screenHeight = (unsigned int)round(roi.height()*resolution.y);

	LOG_ALL(canvaspainterlog) << "estimated screen resolution: " << _screenWidth << ", " << _screenHeight << std::endl;

	int textureWidth  = _screenWidth  + _prefetchLeft + _prefetchRight;
	int textureHeight = _screenHeight + _prefetchTop  + _prefetchBottom;

	LOG_ALL(canvaspainterlog) << "canvas texture has to be of size: " << textureWidth << ", " << textureHeight << std::endl;

	if (prepareTexture(textureWidth, textureHeight))
		initiateFullRedraw(roi, resolution);

	// roi did not change
	if (roi == _previousRoi) {

		LOG_ALL(canvaspainterlog) << "roi did not change -- I just quickly update the strokes" << std::endl;

		updateStrokes(*_strokes, roi, true);

	// size of roi changed -- texture needs to be redrawn completely
	} else if (sizeChanged(roi, _previousRoi)) {

		LOG_ALL(canvaspainterlog) << "roi changed in size" << std::endl;

		initiateFullRedraw(roi, resolution);
		updateStrokes(*_strokes, roi, true);

	// roi was moved
	} else {

		LOG_ALL(canvaspainterlog) << "roi was moved by " << (roi.upperLeft() - _previousRoi.upperLeft()) << std::endl;

		// show a different part of the canvas texture
		shiftTexture(roi.upperLeft() - _previousRoi.upperLeft());
		//updateStrokes(*_strokes, roi, true);
	}

	// TODO: do this in a separate thread
	cleanDirtyAreas();

	drawTexture(roi);

	_previousRoi = roi;
}

bool
CanvasPainter::prepareTexture(int textureWidth, int textureHeight) {

	if (_canvasTexture != 0 && (_canvasTexture->width() != textureWidth || _canvasTexture->height() != textureHeight)) {

		LOG_ALL(canvaspainterlog) << "canvas texture is of different size, create a new one" << std::endl;

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
	_drawnUntilStroke = 0;
	_drawnUntilStrokePoint = 0;
}

void
CanvasPainter::updateStrokes(const Strokes& strokes, const util::rect<double>& roi, bool incremental) {

	if (incremental) {

		// is it necessary to draw something?
		if (_drawnUntilStroke > 1 && _drawnUntilStroke == strokes.size() && _drawnUntilStrokePoint == strokes[_drawnUntilStroke - 1].size())
			return;
	}

	// map the texture memory
	gui::cairo_pixel_t* data = _canvasTexture->map<gui::cairo_pixel_t>();

	drawStrokes(
			data,
			_canvasTexture->width(),
			_canvasTexture->height(),
			strokes,
			roi,
			_textureArea,
			_splitCenter,
			incremental);

	// unmap the texture memory
	_canvasTexture->unmap<gui::cairo_pixel_t>();
}

void
CanvasPainter::drawStrokes(
		gui::cairo_pixel_t* data,
		unsigned int width,
		unsigned int height,
		const Strokes& strokes,
		const util::rect<double>& roi,
		const util::rect<double>& dataArea,
		const util::point<double>& splitCenter,
		bool incremental) {

	LOG_ALL(canvaspainterlog) << "drawing strokes with roi " << roi << std::endl;

	// wrap the buffer in a cairo surface
	// TODO: free surface?
	_surface =
			cairo_image_surface_create_for_data(
					(unsigned char*)data,
					CAIRO_FORMAT_ARGB32,
					width,
					height,
					cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width));

	// create a context for the surface
	// TODO: free context?
	_context = cairo_create(_surface);

	// prepare the background, if we do not draw incrementally
	// TODO: remove, as soon as we use CairoCanvasPainter
	if (!incremental || (_drawnUntilStroke == 0 && _drawnUntilStrokePoint == 0))
		clearSurface();

	// if there is nothing to draw, we are done here
	if (strokes.size() == 0)
		return;

	// Now, we have a surface of width x height, with (0,0) being the upper left 
	// corner and (width-1,height-1) the lower right. Scale and translate 
	// operations, such that the upper left is dataArea.upperLeft() and 
	// lower right is dataArea.lowerRight().

	// scale the texture area diagonal to (width, height)
	util::point<double> scale = dataArea.lowerRight() - dataArea.upperLeft();
	scale.x = width/scale.x;
	scale.y = height/scale.y;
	cairo_scale(_context, scale.x, scale.y);

	// translate dataArea.upperLeft() to (0,0)
	util::point<double> translate = -dataArea.upperLeft();
	cairo_translate(_context, translate.x, translate.y);

	LOG_ALL(canvaspainterlog) << "cairo scale    : " << scale << std::endl;
	LOG_ALL(canvaspainterlog) << "cairo translate: " << translate << std::endl;

	// Now we could start drawing onto the surface, if the ROI is perfectly 
	// centered.  However, in general, there is an additional shift with 
	// wrap-around that we have to compensate for. This gives four parts of the 
	// data that have to be drawn individually.

	// a rectangle used to clip the cairo operations
	util::rect<double> clip;

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

		// draw to texture part
		cairo_translate(_context, translate.x, translate.y);

		// clip outside our responsibility
		cairo_rectangle(_context, clip.minX, clip.minY, clip.width(), clip.height());
		cairo_clip(_context);

		// draw the new strokes in the current part
		// TODO: replace with call to CairoCanvasPainter
		for (unsigned int i = (incremental ? _drawnUntilStroke : 0); i < strokes.size(); i++)
			drawStroke(_context, strokes[i], incremental);

		cairo_restore(_context);
	}

	if (incremental) {

		// remember what we drew already
		_drawnUntilStroke = strokes.size() - 1;
		_drawnUntilStrokePoint = strokes[_drawnUntilStroke].size() - 1;
	}
}

// TODO: remove, as soon as we use CairoCanvasPainter
void
CanvasPainter::drawStroke(cairo_t* context, const Stroke& stroke, bool incremental) {

	if (stroke.size() == 0)
		return;

	// TODO: read this from stroke data structure
	double penWidth = 2.0;
	double penColorRed   = 0;
	double penColorGreen = 0;
	double penColorBlue  = 0;

	cairo_set_line_cap(context, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(context, CAIRO_LINE_JOIN_ROUND);

	for (unsigned int i = (incremental ? _drawnUntilStrokePoint : 0) + 1; i < stroke.size(); i++) {

		double alpha = alphaPressureCurve(stroke[i].pressure);
		double width = widthPressureCurve(stroke[i].pressure);

		cairo_set_source_rgba(context, penColorRed, penColorGreen, penColorBlue, alpha);
		cairo_set_line_width(context, width*penWidth);

		cairo_move_to(context, stroke[i-1].position.x, stroke[i-1].position.y);
		cairo_line_to(context, stroke[i  ].position.x, stroke[i  ].position.y);
		cairo_stroke(context);
	}
}

void
CanvasPainter::drawTexture(const util::rect<double>& roi) {

	// draw the texture
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	_canvasTexture->bind();

	// The texture's ROI is in general split into four parts that we have to 
	// draw individually.

	// the position of the rectangle to draw
	util::rect<double>  position;

	// the translation for the texCoords
	util::point<double> translate;

	// the texCoords
	util::rect<double>  texCoords;

	for (int part = 0; part < 4; part++) {

		LOG_ALL(canvaspainterlog) << "drawing texture part " << part << std::endl;

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

		// translate is in device units, use it with position to determine the 
		// texCoords
		texCoords = position - translate - _textureArea.upperLeft();
		texCoords.minX /= _textureArea.width();
		texCoords.maxX /= _textureArea.width();
		texCoords.minY /= _textureArea.height();
		texCoords.maxY /= _textureArea.height();

		LOG_ALL(canvaspainterlog) << "\ttexture coordinates are " << texCoords << std::endl;
		LOG_ALL(canvaspainterlog) << "\tposition is " << position << std::endl;

		glColor3f(1.0f, 1.0f, 1.0f);
		glBegin(GL_QUADS);
		glTexCoord2d(texCoords.minX, texCoords.minY); glVertex2d(position.minX, position.minY);
		glTexCoord2d(texCoords.maxX, texCoords.minY); glVertex2d(position.maxX, position.minY);
		glTexCoord2d(texCoords.maxX, texCoords.maxY); glVertex2d(position.maxX, position.maxY);
		glTexCoord2d(texCoords.minX, texCoords.maxY); glVertex2d(position.minX, position.maxY);
		glEnd();
	}

	// DEBUG
	util::rect<double> debug = roi*0.25;
	debug += roi.upperLeft() - debug.upperLeft();
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
CanvasPainter::shiftTexture(const util::point<double>& shift) {

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

	util::rect<double> dirtyX;
	util::rect<double> dirtyY;

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

	LOG_ALL(canvaspainterlog) << "texture area is now " << _textureArea << ", split center is at " << _splitCenter << std::endl;
}

void
CanvasPainter::markDirty(const util::rect<double>& area) {

	LOG_ALL(canvaspainterlog) << "area  " << area << " is dirty, now" << std::endl;

	_dirtyAreas.push_back(area);
}

void
CanvasPainter::cleanDirtyAreas() {

	int textureWidth  = _canvasTexture->width();
	int textureHeight = _canvasTexture->height();

	// the buffer's width and height in device units
	double bufferWidth  = (static_cast<double>(_bufferWidth)/textureWidth)*_textureArea.width();
	//double bufferHeight = (static_cast<double>(_bufferHeight)/textureHeight)*_textureArea.height();

	for (unsigned int i = 0; i < _dirtyAreas.size(); i++) {

		util::rect<double>& area = _dirtyAreas[i];

		LOG_ALL(canvaspainterlog) << "cleaning area " << area << std::endl;

		if (area.width() < area.height()) {

			// should be handled by x-buffer
			LOG_ALL(canvaspainterlog) << "cleaning it with x-buffer" << std::endl;

			// process in stripes
			while (area.width() > 0) {

				// put buffer area at left side of dirty area
				util::rect<double> bufferArea;
				bufferArea.minX = area.minX;
				bufferArea.maxX = area.minX + bufferWidth;
				bufferArea.minY = _textureArea.minY;
				bufferArea.maxY = _textureArea.maxY;

				// move it left, if it leaves the texture area
				if (bufferArea.maxX >= _textureArea.maxX)
					bufferArea += util::point<double>(_textureArea.maxX - bufferArea.maxX, 0.0);

				LOG_ALL(canvaspainterlog) << "will clean it with stripe at " << bufferArea << std::endl;

				// map buffer
				gui::cairo_pixel_t* data = _canvasBufferX->map<gui::cairo_pixel_t>();

				util::point<double> splitCenter = _splitCenter;
				if (splitCenter.x < bufferArea.minX)
					splitCenter.x = bufferArea.minX;
				if (splitCenter.x > bufferArea.maxX)
					splitCenter.x = bufferArea.maxX;

				drawStrokes(
						data,
						_bufferWidth,
						textureHeight,
						*_strokes,
						bufferArea, // we have to draw everywhere, since we don't know the previous contents
						bufferArea,
						splitCenter,
						false);

				// unmap buffer
				_canvasBufferX->unmap();

				// update texture
				double offset = bufferArea.minX - _splitCenter.x;
				if (offset < 0)
					offset += _textureArea.width();

				unsigned int pixelOffset = (offset*textureWidth)/_textureArea.width();
				LOG_ALL(canvaspainterlog) << "pixel offset for this stripe is " << pixelOffset << std::endl;

				if (pixelOffset > textureWidth - _bufferWidth) {

					LOG_ERROR(canvaspainterlog) << "pixel offset for x-buffer stripe is too big: " << pixelOffset << std::endl;
					pixelOffset = textureWidth - _bufferWidth;
				}

				_canvasTexture->loadData(*_canvasBufferX, pixelOffset, 0);

				// remove updated part from dirty area
				area.minX += bufferWidth;
			}
		}
	}

	_dirtyAreas.clear();
}

void
CanvasPainter::initiateFullRedraw(const util::rect<double>& roi, const util::point<double>& resolution) {

	// draw all strokes, the next time
	_drawnUntilStroke = 0;
	_drawnUntilStrokePoint = 0;

	// recompute area represented by canvas texture and buffers
	_textureArea.minX = roi.minX - _prefetchLeft*(1.0/resolution.x);
	_textureArea.minY = roi.minY - _prefetchTop*(1.0/resolution.y);
	_textureArea.maxX = roi.maxX + _prefetchRight*(1.0/resolution.x);
	_textureArea.maxY = roi.maxY + _prefetchBottom*(1.0/resolution.y);

	// center the current roi in the texture
	_splitCenter = _textureArea.upperLeft();
}

void
CanvasPainter::clearSurface() {

	// clear surface
	cairo_set_operator(_context, CAIRO_OPERATOR_CLEAR);
	cairo_paint(_context);
	cairo_set_operator(_context, CAIRO_OPERATOR_OVER);
}

bool
CanvasPainter::sizeChanged(const util::rect<double>& roi, const util::rect<double>& previousRoi) {

	double relChangeWidth  = roi.width()/previousRoi.width();
	double relChangeHeight = roi.height()/previousRoi.height();

	// if either of the dimensions changed by at least 0.01%
	if (std::max(std::abs(1.0 - relChangeWidth), std::abs(1.0 - relChangeHeight)) > 0.0001)
		return true;

	return false;
}

// TODO: remove, as soon as we are using CairoCanvasPainter
double
CanvasPainter::widthPressureCurve(double pressure) {

	const double minPressure = 0.0;
	const double maxPressure = 1.5;

	pressure /= 2048.0;

	return minPressure + pressure*(maxPressure - minPressure);
}

// TODO: remove, as soon as we are using CairoCanvasPainter
double
CanvasPainter::alphaPressureCurve(double pressure) {

	const double minPressure = 1;
	const double maxPressure = 1;

	pressure /= 2048.0;

	return minPressure + pressure*(maxPressure - minPressure);
}
