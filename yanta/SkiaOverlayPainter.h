#ifndef YANTA_SKIA_OVERLAY_PAINTER_H__
#define YANTA_SKIA_OVERLAY_PAINTER_H__

#include <SkCanvas.h>
#include <util/point.hpp>
#include <util/rect.hpp>
#include "Overlay.h"
#include "SkiaOverlayObjectPainter.h"

class SkiaOverlayPainter {

public:

	void setOverlay(boost::shared_ptr<Overlay> overlay) { _overlay = overlay; }

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
	 * Draw the whole canvas on the provided context.
	 */
	void draw(SkCanvas& context);

	/**
	 * Draw the canvas in the given ROI on the provided context. If 
	 * drawnUntilStroke is non-zero, only an incremental draw is performed, 
	 * starting from stroke drawnUntilStroke with point drawnUntilStrokePoint.
	 */
	void draw(
			SkCanvas& context,
			const util::rect<CanvasPrecision>& roi);

private:

	boost::shared_ptr<Overlay> _overlay;

	util::point<double> _pixelsPerDeviceUnit;
	util::point<int>    _pixelOffset;
};

#endif // YANTA_SKIA_OVERLAY_PAINTER_H__

