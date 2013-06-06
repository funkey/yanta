#ifndef SPLINES_CAIRO_CANVAS_PAINTER_H__
#define SPLINES_CAIRO_CANVAS_PAINTER_H__

#include <cairo.h>

#include <util/rect.hpp>
#include "Strokes.h"

class CairoCanvasPainter {

public:

	CairoCanvasPainter();

	void setStrokes(boost::shared_ptr<Strokes> strokes) { _strokes = strokes; }

	/**
	 * Set the transformation to map from canvas pixel coordinates to device 
	 * units.
	 */
	void setDeviceTransformation(
			const util::point<double>& pixelsPerDeviceUnit,
			const util::point<double>& pixelOffset) {

		_pixelsPerDeviceUnit = pixelsPerDeviceUnit;
		_pixelOffset         = pixelOffset;
	}

	/**
	 * Draw the whole canvas on the provided context.
	 */
	void draw(cairo_t* context);

	/**
	 * Draw the canvas in the given ROI on the provided context. If 
	 * drawnUntilStroke is non-zero, only an incremental draw is performed, 
	 * starting from stroke drawnUntilStroke with point drawnUntilStrokePoint.
	 */
	void draw(
			cairo_t* context,
			const util::rect<double>& roi,
			unsigned int drawnUntilStroke = 0,
			unsigned int drawnUntilStrokePoint = 0);

private:

	void clearSurface(cairo_t* context);

	void drawStroke(
			cairo_t* context,
			const Stroke& stroke,
			const util::rect<double>& roi,
			unsigned int drawnUntilStrokePoint);

	double widthPressureCurve(double pressure);

	double alphaPressureCurve(double pressure);

	boost::shared_ptr<Strokes> _strokes;

	util::point<double> _pixelsPerDeviceUnit;
	util::point<double> _pixelOffset;
};

#endif // SPLINES_CAIRO_CANVAS_PAINTER_H__

