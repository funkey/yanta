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

bool
SkiaCanvasPainter::alreadyDrawn(const Strokes& strokes) {

	// is it necessary to draw something?
	if (_drawnUntilStroke > 1 &&
	    _drawnUntilStroke == strokes.size() &&
	    _drawnUntilStrokePoint == strokes[_drawnUntilStroke - 1].size() - (strokes[_drawnUntilStroke - 1].finished() ? 1 : 0))

		return true;

	return false;
}

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

	if (_strokes->size() > 0) {

		Stroke& stroke = (*_strokes)[_drawnUntilStrokeTmp];
		_drawnUntilStrokePointTmp = std::max(0, static_cast<int>(stroke.size()) - (stroke.finished() ? 1 : 2));

	} else
		_drawnUntilStrokePointTmp = 0;
}

void
SkiaCanvasPainter::clearSurface(SkCanvas& canvas) {

	// clear the entire surface, irrespective of clipping
	canvas.clear(SkColorSetRGB(255, 255, 255));
}

void
SkiaCanvasPainter::drawStroke(
		SkCanvas& canvas,
		const Stroke& stroke,
		const util::rect<double>& /*roi*/,
		unsigned int drawnUntilStrokePoint) {

	if (stroke.size() <= drawnUntilStrokePoint + 1)
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

	double length;
	util::point<double> prevLineNormal    = getLineNormal(stroke, (int)drawnUntilStrokePoint - 1, length);
	util::point<double> currentLineNormal = getLineNormal(stroke, drawnUntilStrokePoint, length);
	util::point<double> prevCornerNormal  = 0.5*(prevLineNormal + currentLineNormal);

	// the factor by which to enlarge the stroke segments to avaid antialising 
	// lines between them
	const double eps = 1e-2;

	// for each line in the stroke
	for (unsigned int i = drawnUntilStrokePoint; i < stroke.size() - (stroke.finished() ? 1 : 2); i++) {

		//double alpha = alphaPressureCurve(stroke[i].pressure);
		double widthStart = 0.5*widthPressureCurve(stroke[i  ].pressure)*penWidth;
		double widthEnd   = 0.5*widthPressureCurve(stroke[i+1].pressure)*penWidth;

		const util::point<double>& begin = stroke[i].position;
		const util::point<double>& end   = stroke[i+1].position;

		util::point<double> nextLineNormal   = getLineNormal(stroke, i+1, length);
		util::point<double> nextCornerNormal = 0.5*(currentLineNormal + nextLineNormal);

		SkPath path;
		path.moveTo(
				begin.x + widthStart*prevCornerNormal.x - prevCornerNormal.y*eps*length,
				begin.y + widthStart*prevCornerNormal.y + prevCornerNormal.x*eps*length);
		path.lineTo(
				end.x + widthEnd*nextCornerNormal.x + nextCornerNormal.y*eps*length,
				end.y + widthEnd*nextCornerNormal.y - nextCornerNormal.x*eps*length);
		path.lineTo(
				end.x - widthEnd*nextCornerNormal.x + nextCornerNormal.y*eps*length,
				end.y - widthEnd*nextCornerNormal.y - nextCornerNormal.x*eps*length);
		path.lineTo(
				begin.x - widthStart*prevCornerNormal.x - prevCornerNormal.y*eps*length,
				begin.y - widthStart*prevCornerNormal.y + prevCornerNormal.x*eps*length);
		path.lineTo(
				begin.x + widthStart*prevCornerNormal.x - prevCornerNormal.y*eps*length,
				begin.y + widthStart*prevCornerNormal.y + prevCornerNormal.x*eps*length);

		canvas.drawPath(path, paint);

		currentLineNormal = nextLineNormal;
		prevCornerNormal  = nextCornerNormal;
	}
}

util::point<double>
SkiaCanvasPainter::getLineNormal(const Stroke& stroke, int i, double& length) {

	if (i < 0 || i >= (int)stroke.size() - 1)
		return util::point<double>(0, 0);

	util::point<double> begin = stroke[i].position;
	util::point<double> end   = stroke[i+1].position;

	util::point<double> diff = end - begin;

	length = sqrt(diff.x*diff.x + diff.y*diff.y);

	util::point<double> normal;
	normal.x = -diff.y/length;
	normal.y =  diff.x/length;

	return normal;
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
