#ifndef YANTA_SKIA_CANVAS_PAINTER_H__
#define YANTA_SKIA_CANVAS_PAINTER_H__

#include <SkCanvas.h>

#include <gui/Skia.h>
#include <util/rect.hpp>
#include "Canvas.h"

class SkiaCanvasPainter {

public:

	SkiaCanvasPainter(gui::skia_pixel_t clearColor);

	void setCanvas(boost::shared_ptr<Canvas> canvas) { _canvas = canvas; }

	/**
	 * Set the transformation to map from canvas units to pixel units.
	 */
	void setDeviceTransformation(
			const util::point<double>& pixelsPerDeviceUnit,
			const util::point<int>&    pixelOffset) {

		_pixelsPerDeviceUnit = pixelsPerDeviceUnit;
		_pixelOffset         = pixelOffset;
	}

	/**
	 * Reset the memory about what has been drawn already. Call this method to 
	 * re-initialize incremental drawing.
	 */
	void resetIncrementalMemory() {

		_drawnUntilStrokePoint = 0;
	}

	void rememberDrawnCanvas() {

		_drawnUntilStrokePoint = _drawnUntilStrokePointTmp;
	}

	/**
	 * Return true if all the strokes of this canvas have been drawn by this 
	 * painter already.
	 */
	bool alreadyDrawn(const Canvas& canvas);

	/**
	 * Draw the whole canvas on the provided context.
	 */
	void draw(SkCanvas& canvas);

	/**
	 * Draw the canvas in the given ROI on the provided context. If 
	 * drawnUntilStroke is non-zero, only an incremental draw is performed, 
	 * starting from stroke drawnUntilStroke with point drawnUntilStrokePoint.
	 */
	void draw(
			SkCanvas& context,
			const util::rect<CanvasPrecision>& roi);

private:

	void clearSurface(SkCanvas& context);

	bool drawStroke(
			SkCanvas& context,
			const Stroke& stroke,
			const util::rect<CanvasPrecision>& roi,
			unsigned long beginStroke,
			unsigned long endStroke);

	util::point<double> getLineNormal(const Stroke& stroke, const StrokePoints& points, long i, double& length);

	double widthPressureCurve(double pressure);

	double alphaPressureCurve(double pressure);

	gui::skia_pixel_t _clearColor;

	boost::shared_ptr<Canvas> _canvas;

	util::point<double> _pixelsPerDeviceUnit;
	util::point<int>    _pixelOffset;

	// the number of the stroke point until which all have been drawn already
	unsigned long _drawnUntilStrokePoint;

	// temporal memory of the number of the stroke point until which all have 
	// been drawn already
	unsigned long _drawnUntilStrokePointTmp;
};

#endif // YANTA_SKIA_CANVAS_PAINTER_H__

