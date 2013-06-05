#include <cmath>

#include <gui/OpenGl.h>
#include <util/Logger.h>
#include "CanvasPainter.h"

logger::LogChannel canvaspainterlog("canvaspainterlog", "[CanvasPainter] ");

CanvasPainter::CanvasPainter() :
	_canvasTexture(0),
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

	if (_canvasTexture != 0 && (_canvasTexture->width() != textureWidth || _canvasTexture->height() != textureHeight)) {

		LOG_ALL(canvaspainterlog) << "canvas texture is of different size, create a new one" << std::endl;

		delete _canvasTexture;
		_canvasTexture = 0;
	}

	if (_canvasTexture == 0) {

		_canvasTexture = new gui::Texture(textureWidth, textureHeight, GL_RGBA /* TODO: should this be the format value of the opengl tratis for cairo_pixel_t? */);
		initiateFullRedraw(roi, resolution);
	}

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
		updateStrokes(*_strokes, roi, true);
	}

	drawTexture(roi);

	// TODO: do this in a separate thread
	cleanDirtyAreas();

	_previousRoi = roi;
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

	drawStrokes(data, strokes, roi, incremental);

	// unmap the texture memory
	_canvasTexture->unmap<gui::cairo_pixel_t>();
}

void
CanvasPainter::drawStrokes(
		gui::cairo_pixel_t* data,
		const Strokes& strokes,
		const util::rect<double>& roi,
		bool incremental) {

	LOG_ALL(canvaspainterlog) << "drawing strokes with roi " << roi << std::endl;

	GLdouble  width  = _canvasTexture->width();
	GLdouble  height = _canvasTexture->height();

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
	if (!incremental || (_drawnUntilStroke == 0 && _drawnUntilStrokePoint == 0))
		clearSurface();

	// if there is nothing to draw, we are done here
	if (strokes.size() == 0)
		return;

	// Now, we have a surface of width x height, with (0,0) being the upper left 
	// corner and (width-1,height-1) the lower right. Scale and translate 
	// operations, such that the upper left is _textureArea.upperLeft() and 
	// lower right is _textureArea.lowerRight().

	// scale the texture area diagonal to (width, height)
	util::point<double> scale = _textureArea.lowerRight() - _textureArea.upperLeft();
	scale.x = width/scale.x;
	scale.y = height/scale.y;
	cairo_scale(_context, scale.x, scale.y);

	// translate _textureArea.upperLeft() to (0,0)
	util::point<double> translate = -_textureArea.upperLeft();
	cairo_translate(_context, translate.x, translate.y);

	LOG_ALL(canvaspainterlog) << "cairo scale    : " << scale << std::endl;
	LOG_ALL(canvaspainterlog) << "cairo translate: " << translate << std::endl;

	// Now we could start drawing onto the surface, if the _textureRoi is 
	// perfectly centered.  However, in general, there is an additional shift 
	// with wrap-around that we have to compensate for. This gives four parts of 
	// the texture that have to be drawn individually.

	// a rectangle used to clip the cairo operations
	util::rect<double> clip;

	for (int part = 0; part < 4; part++) {

		switch (part) {

			// upper left
			case 0:

				clip.minX = _splitCenter.x;
				clip.minY = _splitCenter.y;
				clip.maxX = _textureArea.maxX;
				clip.maxY = _textureArea.maxY;

				translate = _textureArea.upperLeft() - _splitCenter;
				break;

			// upper right
			case 1:

				clip.minX = _textureArea.minX;
				clip.minY = _splitCenter.y;
				clip.maxX = _splitCenter.x;
				clip.maxY = _textureArea.maxY;

				translate = _textureArea.upperRight() - _splitCenter;
				break;

			// lower left
			case 2:

				clip.minX = _splitCenter.x;
				clip.minY = _textureArea.minY;
				clip.maxX = _textureArea.maxX;
				clip.maxY = _splitCenter.y;

				translate = _textureArea.lowerLeft() - _splitCenter;
				break;

			// lower right
			case 3:

				clip.minX = _textureArea.minX;
				clip.minY = _textureArea.minY;
				clip.maxX = _splitCenter.x;
				clip.maxY = _splitCenter.y;

				translate = _textureArea.lowerRight() - _splitCenter;
				break;
		}

		cairo_save(_context);

		// draw to texture part
		cairo_translate(_context, translate.x, translate.y);

		// clip outside our responsibility
		cairo_rectangle(_context, clip.minX, clip.minY, clip.width(), clip.height());
		cairo_clip(_context);

		// draw the new strokes in the current part
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

		// DEBUG
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glBegin(GL_QUADS);
		glColor4f(1.0f/(part+1), 1.0f/(4-part+1), 1.0f, 0.1f);
		glVertex2d(position.minX, position.minY);
		glVertex2d(position.maxX, position.minY);
		glVertex2d(position.maxX, position.maxY);
		glVertex2d(position.minX, position.maxY);
		glEnd();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		// END DEBUG

		glColor3f(1.0f, 1.0f, 1.0f);
		glBegin(GL_QUADS);
		glTexCoord2d(texCoords.minX, texCoords.minY); glVertex2d(position.minX, position.minY);
		glTexCoord2d(texCoords.maxX, texCoords.minY); glVertex2d(position.maxX, position.minY);
		glTexCoord2d(texCoords.maxX, texCoords.maxY); glVertex2d(position.maxX, position.maxY);
		glTexCoord2d(texCoords.minX, texCoords.maxY); glVertex2d(position.minX, position.maxY);
		glEnd();
	}

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

	_dirtyAreas.push_back(area);
}

void
CanvasPainter::cleanDirtyAreas() {

	for (unsigned int i = 0; i < _dirtyAreas.size(); i++)
		updateStrokes(*_strokes, _dirtyAreas[i], false);

	_dirtyAreas.clear();
}

void
CanvasPainter::initiateFullRedraw(const util::rect<double>& roi, const util::point<double>& resolution) {

	// draw all strokes, the next time
	_drawnUntilStroke = 0;
	_drawnUntilStrokePoint = 0;

	// recompute are represented by canvas texture
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

double
CanvasPainter::widthPressureCurve(double pressure) {

	const double minPressure = 0.0;
	const double maxPressure = 1.5;

	pressure /= 2048.0;

	return minPressure + pressure*(maxPressure - minPressure);
}

double
CanvasPainter::alphaPressureCurve(double pressure) {

	const double minPressure = 1;
	const double maxPressure = 1;

	pressure /= 2048.0;

	return minPressure + pressure*(maxPressure - minPressure);
}
