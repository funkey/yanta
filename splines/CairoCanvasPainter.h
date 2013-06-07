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
	 * Enable or disable incremental mode. In this mode, the painter remembers 
	 * what it drew already and subsequent calls to draw() will only add what's 
	 * new.
	 */
	void setIncremental(bool incremental) {

		_incremental = incremental;
	}

	/**
	 * Reset the memory about what has been drawn already. Call this method to 
	 * re-initialize incremental drawing.
	 */
	void resetIncrementalMemory() {

		_drawnUntilStroke      = 0;
		_drawnUntilStrokePoint = 0;
	}

	void rememberDrawnStrokes() {

		_drawnUntilStroke = _drawnUntilStrokeTmp;
		_drawnUntilStrokePoint = _drawnUntilStrokePointTmp;
	}

	/**
	 * Get the number of the stroke until which all previous strokes have been 
	 * drawn already (excluding the stroke represented by the number).
	 */
	unsigned int drawnUntilStroke()      { return _drawnUntilStroke; }
	unsigned int drawnUntilStrokePoint() { return _drawnUntilStrokePoint; }

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
			const util::rect<double>& roi);

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

	// draw only updates
	bool _incremental;

	// the number of the stroke until which all have been drawn already
	unsigned int _drawnUntilStroke;
	unsigned int _drawnUntilStrokePoint;

	// temporal memory of the number of the stroke until which all have been 
	// drawn already
	unsigned int _drawnUntilStrokeTmp;
	unsigned int _drawnUntilStrokePointTmp;
};

#endif // SPLINES_CAIRO_CANVAS_PAINTER_H__

