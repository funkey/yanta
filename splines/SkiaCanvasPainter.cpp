#include <util/Logger.h>
#include "SkiaCanvasPainter.h"

logger::LogChannel skiacanvaspainterlog("skiacanvaspainterlog", "[SkiaCanvasPainter] ");

SkiaCanvasPainter::SkiaCanvasPainter() :
	_pixelsPerDeviceUnit(1.0, 1.0),
	_pixelOffset(0, 0),
	_drawnUntilStroke(0),
	_drawnUntilStrokePoint(0),
	_drawnUntilStrokeTmp(0),
	_drawnUntilStrokePointTmp(0) {}

void
SkiaCanvasPainter::draw(SkCanvas& canvas) {

	util::rect<double> roi(0, 0, 0, 0);
	draw(canvas, roi);
}

void
SkiaCanvasPainter::draw(
		SkCanvas& canvas,
		const util::rect<double>& roi) {

	bool incremental = (_drawnUntilStroke != 0 || _drawnUntilStrokePoint != 0);

	LOG_ALL(skiacanvaspainterlog) << "drawing " << (incremental ? "" : "non-") << "incrementally" << std::endl;

	canvas.save();

	util::rect<double> canvasRoi(0, 0, 0, 0);

	if (roi.area() != 0) {

		// clip outside our responsibility
		canvas.clipRect(SkRect::MakeLTRB(roi.minX, roi.minY, roi.maxX, roi.maxY));

		// transform roi to canvas units
		canvasRoi = roi;
		canvasRoi -= _pixelOffset;
		canvasRoi /= _pixelsPerDeviceUnit;
	}

	// apply the given transformation

	// translate should be performed...
	canvas.translate(_pixelOffset.x, _pixelOffset.y);
	// ...after the scaling
	canvas.scale(_pixelsPerDeviceUnit.x, _pixelsPerDeviceUnit.y);

	// prepare the background, if we do not draw incrementally
	if (!incremental)
		clearSurface(canvas);

	// draw the (new) strokes in the current part
	unsigned int drawnUntilStrokePoint = (incremental ? _drawnUntilStrokePoint : 0);
	for (unsigned int i = (incremental ? _drawnUntilStroke : 0); i < _strokes->size(); i++) {

		LOG_ALL(skiacanvaspainterlog)
				<< "drawing stroke " << i << ", starting from point "
				<< drawnUntilStrokePoint << std::endl;

		const Stroke& stroke = (*_strokes)[i];

		// draw only of no roi given or stroke intersects roi
		if (canvasRoi.width() == 0 || stroke.boundingBox().intersects(canvasRoi))
			drawStroke(canvas, stroke, canvasRoi, drawnUntilStrokePoint);

		drawnUntilStrokePoint = 0;
	}

	canvas.restore();

	// temporarilly remember what we drew already
	_drawnUntilStrokeTmp = std::max(0, static_cast<int>(_strokes->size()) - 1);

	if (_strokes->size() > 0)
		_drawnUntilStrokePointTmp = std::max(0, static_cast<int>((*_strokes)[_drawnUntilStrokeTmp].size()) - 1);
	else
		_drawnUntilStrokePointTmp = 0;
}

void
SkiaCanvasPainter::drawStroke(
		SkCanvas& canvas,
		const Stroke& stroke,
		const util::rect<double>& /*roi*/,
		unsigned int drawnUntilStrokePoint) {

	if (stroke.size() == 0)
		return;

	// TODO: read this from stroke data structure
	double penWidth = stroke.width();
	unsigned char penColorRed   = 0;
	unsigned char penColorGreen = 0;
	unsigned char penColorBlue  = 0;

	SkPaint paint;
	paint.setStrokeCap(SkPaint::kRound_Cap);
	paint.setColor(SkColorSetRGB(penColorRed, penColorGreen, penColorBlue));
	paint.setAntiAlias(true);

	for (unsigned int i = drawnUntilStrokePoint + 1; i < stroke.size(); i++) {

		//double alpha = alphaPressureCurve(stroke[i].pressure);
		double width = widthPressureCurve(stroke[i].pressure);

		paint.setStrokeWidth(width*penWidth);

		canvas.drawLine(
			stroke[i-1].position.x, stroke[i-1].position.y,
			stroke[i  ].position.x, stroke[i  ].position.y,
			paint);
	}
}

void
SkiaCanvasPainter::clearSurface(SkCanvas& canvas) {

	// clear the entire surface, irrespective of clipping
	canvas.clear(SkColorSetRGB(255, 255, 255));
}

double
SkiaCanvasPainter::widthPressureCurve(double pressure) {

	const double minPressure = 0.0;
	const double maxPressure = 1.5;

	pressure /= 2048.0;

	return minPressure + pressure*(maxPressure - minPressure);
}

double
SkiaCanvasPainter::alphaPressureCurve(double pressure) {

	const double minPressure = 1;
	const double maxPressure = 1;

	pressure /= 2048.0;

	return minPressure + pressure*(maxPressure - minPressure);
}
