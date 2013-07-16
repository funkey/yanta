#include "SkiaOverlayPainter.h"

void
SkiaOverlayPainter::draw(SkCanvas& canvas) {

	util::rect<CanvasPrecision> roi(0, 0, 0, 0);
	draw(canvas, roi);
}

void
SkiaOverlayPainter::draw(
		SkCanvas& canvas,
		const util::rect<CanvasPrecision>& roi) {

	canvas.save();

	// clear the surface, respecting the clipping
	canvas.clear(SkColorSetARGB(0, 255, 255, 255));

	util::rect<double> canvasRoi(0, 0, 0, 0);

	if (!roi.isZero()) {

		// clip outside our responsibility
		canvas.clipRect(SkRect::MakeLTRB(roi.minX, roi.minY, roi.maxX, roi.maxY));

		// transform roi to canvas units
		canvasRoi = roi;
		canvasRoi -= _pixelOffset; // TODO: precision-problematic conversion
		canvasRoi /= _pixelsPerDeviceUnit;
	}

	// apply the given transformation

	// translate should be performed...
	canvas.translate(_pixelOffset.x, _pixelOffset.y); // TODO: precision-problematic conversion
	// ...after the scaling
	canvas.scale(_pixelsPerDeviceUnit.x, _pixelsPerDeviceUnit.y);

	SkiaOverlayObjectPainter overlayObjectPainter(canvas);

	// draw the overlay objects
	for (unsigned int i = 0; i < _overlay->numObjects(); i++) {

		OverlayObject& overlayObject = _overlay->getObject(i);

		if (overlayObject.getBoundingBox().intersects(canvasRoi))
			overlayObject.visit(overlayObjectPainter);
	}

	canvas.restore();
}
