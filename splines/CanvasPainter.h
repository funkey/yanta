#ifndef CANVAS_PAINTER_H__
#define CANVAS_PAINTER_H__

#include <cairo.h>
#include <gui/Cairo.h>
#include <gui/Painter.h>
#include <gui/Texture.h>

#include "Strokes.h"

class CanvasPainter : public gui::Painter {

public:

	CanvasPainter();

	void setStrokes(boost::shared_ptr<Strokes> strokes) {

		_strokes = strokes;
	}

	void draw(
			const util::rect<double>&  roi,
			const util::point<double>& resolution);

private:

	void drawStrokes(
			gui::cairo_pixel_t* data,
			const Strokes& strokes,
			const util::rect<double>& roi);

	void drawStroke(
			cairo_t* context,
			const Stroke& stroke);

	void initiateFullRedraw();

	void clearSurface();

	/**
	 * Get the width multiplier for a pressure value.
	 */
	double widthPressureCurve(double pressure);

	/**
	 * Get the alpha multiplier for a pressure value.
	 */
	double alphaPressureCurve(double pressure);

	// the strokes to draw
	boost::shared_ptr<Strokes> _strokes;

	// the OpenGl texture to draw to
	gui::Texture* _canvasTexture;

	// the cairo context
	cairo_t*            _context;

	// the cairo surface to draw to
	cairo_surface_t*    _surface;

	// the roi of the last call to draw
	util::rect<double>  _previousRoi;

	// the number of the stroke until which all have been drawn already for the 
	// current configuration
	unsigned int _drawnUntilStroke;
	unsigned int _drawnUntilStrokePoint;
};

#endif // CANVAS_PAINTER_H__

