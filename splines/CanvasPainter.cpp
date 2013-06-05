#include <gui/OpenGl.h>
#include <util/Logger.h>
#include "CanvasPainter.h"

logger::LogChannel canvaspainterlog("canvaspainterlog", "[CanvasPainter] ");

CanvasPainter::CanvasPainter() :
	_canvasTexture(0) {

	initiateFullRedraw();
}

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

	typedef gui::cairo_pixel_t value_type;

	// estimate screen resolution
	int resX = static_cast<int>(roi.width()*resolution.x);
	int resY = static_cast<int>(roi.height()*resolution.y);

	LOG_ALL(canvaspainterlog) << "estimated screen resolution: " << resX << ", " << resY << std::endl;

	if (_canvasTexture != 0 && (_canvasTexture->width() != resX || _canvasTexture->height() != resY)) {

		delete _canvasTexture;
		_canvasTexture = 0;
	}

	if (_canvasTexture == 0) {

		_canvasTexture = new gui::Texture(resX, resY, GL_RGBA /* TODO: should this be the format value of the opengl tratis for cairo_pixel_t? */);
		initiateFullRedraw();
	}

	if (roi != _previousRoi) {

		initiateFullRedraw();
	}
	_previousRoi = roi;

	// map the texture memory
	value_type* data = _canvasTexture->map<value_type>();

	drawStrokes(data, *_strokes, roi);

	// unmap the texture memory
	_canvasTexture->unmap<value_type>();

	// draw the texture
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	_canvasTexture->bind();

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
	glTexCoord2d(0.0, 0.0); glVertex2d(roi.minX, roi.minY); 
	glTexCoord2d(1.0, 0.0); glVertex2d(roi.maxX, roi.minY); 
	glTexCoord2d(1.0, 1.0); glVertex2d(roi.maxX, roi.maxY); 
	glTexCoord2d(0.0, 1.0); glVertex2d(roi.minX, roi.maxY); 
	glEnd();

	glDisable(GL_BLEND);
}

void
CanvasPainter::drawStrokes(
		gui::cairo_pixel_t* data,
		const Strokes& strokes,
		const util::rect<double>& roi) {

	GLdouble  width  = _canvasTexture->width();
	GLdouble  height = _canvasTexture->height();

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

	// Now, we have a surface of width x height, with (0,0) being the upper left 
	// corner and (width-1,height-1) the lower right. Scale and translate 
	// operations, such that the upper left is roi.upperLeft() and lower right 
	// is roi.lowerRight().

	// scale roi diagonal to (width, height)
	util::point<double> scale = roi.lowerRight() - roi.upperLeft();
	scale.x = width/scale.x;
	scale.y = height/scale.y;
	cairo_scale(_context, scale.x, scale.y);

	// translate roi.upperLeft() to (0,0)
	util::point<double> translate = -roi.upperLeft();
	cairo_translate(_context, translate.x, translate.y);

	LOG_ALL(canvaspainterlog) << "cairo scale    : " << scale << std::endl;
	LOG_ALL(canvaspainterlog) << "cairo translate: " << translate << std::endl;

	if (_drawnUntilStroke == 0 && _drawnUntilStrokePoint == 0)
		clearSurface();

	if (strokes.size() == 0)
		return;

	for (unsigned int i = _drawnUntilStroke; i < strokes.size(); i++)
		drawStroke(_context, strokes[i]);

	// remember what we drew already
	_drawnUntilStroke = strokes.size() - 1;
	_drawnUntilStrokePoint = strokes[_drawnUntilStroke].size() - 1;
}

void
CanvasPainter::drawStroke(cairo_t* context, const Stroke& stroke) {

	if (stroke.size() == 0)
		return;

	// TODO: read this from stroke data structure
	double penWidth = 2.0;
	double penColorRed   = 0;
	double penColorGreen = 0;
	double penColorBlue  = 0;

	cairo_set_line_cap(context, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(context, CAIRO_LINE_JOIN_ROUND);

	for (unsigned int i = _drawnUntilStrokePoint + 1; i < stroke.size(); i++) {

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
CanvasPainter::initiateFullRedraw() {

	_drawnUntilStroke = 0;
	_drawnUntilStrokePoint = 0;
}

void
CanvasPainter::clearSurface() {

	// clear surface
	cairo_set_operator(_context, CAIRO_OPERATOR_CLEAR);
	cairo_paint(_context);
	cairo_set_operator(_context, CAIRO_OPERATOR_OVER);
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
