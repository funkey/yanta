#include <util/Logger.h>
#include "SkiaCanvasPainter.h"

logger::LogChannel skiacanvaspainterlog("skiacanvaspainterlog", "[SkiaCanvasPainter] ");

SkiaCanvasPainter::SkiaCanvasPainter(gui::skia_pixel_t clearColor) :
	_clearColor(clearColor),
	_pixelsPerDeviceUnit(1.0, 1.0),
	_pixelOffset(0, 0),
	_drawnUntilStrokePoint(0),
	_drawnUntilStrokePointTmp(0) {}

bool
SkiaCanvasPainter::alreadyDrawn(const Strokes& strokes) {

	// is it necessary to draw something?
	if (strokes.numStrokes() > 0 && _drawnUntilStrokePoint == strokes.currentStroke().end() - (strokes.currentStroke().finished() ? 1 : 0))
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

	bool incremental = (_drawnUntilStrokePoint != 0);

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

	// make sure reading access to the stroke points are safe
	boost::shared_lock<boost::shared_mutex> lock(_strokes->getStrokePoints().getMutex());

	// reset temporal memory about what we drew already
	_drawnUntilStrokePointTmp = 0;

	// draw the (new) strokes in the current part
	for (unsigned int i = 0; i < _strokes->numStrokes(); i++) {

		const Stroke& stroke = _strokes->getStroke(i);

		// the effective end of a stroke is one less, if the stroke is not 
		// finished, yet
		long begin = std::max((long)_drawnUntilStrokePoint - 1, (long)stroke.begin());
		long end   = (long)stroke.end() - (stroke.finished() ? 0 : 1);

		// drawn already?
		if (end <= (long)_drawnUntilStrokePoint) {

			LOG_ALL(skiacanvaspainterlog) << "stroke " << i << " was drawn already" << std::endl;
			continue;
		}

		// draw only of no roi given or stroke intersects roi
		if (canvasRoi.width() == 0 || !stroke.finished() || stroke.boundingBox().intersects(canvasRoi)) {

			LOG_ALL(skiacanvaspainterlog)
					<< "drawing stroke " << i << " (" << stroke.begin() << " - " << stroke.end()
					<< ") , starting from point " << begin << " until " << end << std::endl;

			if (!drawStroke(canvas, stroke, canvasRoi, begin, end))
				continue;

		} else {

			LOG_ALL(skiacanvaspainterlog) << "stroke " << i << " is not visible" << std::endl;
		}

		// remember the biggest point we drew already
		_drawnUntilStrokePointTmp = std::max(_drawnUntilStrokePointTmp, (unsigned long)end);
	}

	canvas.restore();

	LOG_ALL(skiacanvaspainterlog) << "I drew everything until stroke point " << _drawnUntilStrokePointTmp << std::endl;
}

void
SkiaCanvasPainter::clearSurface(SkCanvas& canvas) {

	// clear the surface, respecting the clipping
	canvas.drawColor(SkColorSetRGB(_clearColor.red, _clearColor.green, _clearColor.blue));
}

bool
SkiaCanvasPainter::drawStroke(
		SkCanvas& canvas,
		const Stroke& stroke,
		const util::rect<double>& /*roi*/,
		unsigned long beginStroke,
		unsigned long endStroke) {

	// make sure there are enough points in this stroke to draw it
	if (stroke.end() - stroke.begin() <= 1) {

		LOG_ALL(skiacanvaspainterlog) << "this stroke has less than two points -- skip drawing" << std::endl;
		return false;
	}

	double penWidth = stroke.getPen().width();
	// TODO: read this from stroke data structure
	unsigned char penColorRed   = 0;
	unsigned char penColorGreen = 0;
	unsigned char penColorBlue  = 0;

	const StrokePoints& strokePoints = _strokes->getStrokePoints();

	SkPaint paint;
	paint.setStrokeCap(SkPaint::kRound_Cap);
	paint.setColor(SkColorSetRGB(penColorRed, penColorGreen, penColorBlue));
	paint.setAntiAlias(true);

	double length;
	util::point<double> prevLineNormal    = getLineNormal(stroke, strokePoints, (long)beginStroke - 1, length);
	util::point<double> currentLineNormal = getLineNormal(stroke, strokePoints, beginStroke, length);
	util::point<double> prevCornerNormal  = 0.5*(prevLineNormal + currentLineNormal);

	// the factor by which to enlarge the stroke segments to avaid antialising 
	// lines between them
	const double eps = 1e-2;

	// for each line in the stroke
	for (unsigned long i = beginStroke; i < endStroke - 1; i++) {

		LOG_ALL(skiacanvaspainterlog) << "\tdrawing line " << i << std::endl;

		//double alpha = alphaPressureCurve(stroke[i].pressure);
		double widthStart = 0.5*widthPressureCurve(strokePoints[i  ].pressure)*penWidth;
		double widthEnd   = 0.5*widthPressureCurve(strokePoints[i+1].pressure)*penWidth;

		const util::point<double>& begin = strokePoints[i].position;
		const util::point<double>& end   = strokePoints[i+1].position;

		util::point<double> nextLineNormal   = getLineNormal(stroke, strokePoints, i+1, length);
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

	return true;
}

util::point<double>
SkiaCanvasPainter::getLineNormal(const Stroke& stroke, const StrokePoints& points, long i, double& length) {

	// i is the line number, i.e., the line (i,i+1). Now, if i or i+1 is not 
	// part of the stroke, the normal will be zero
	if (i < (long)stroke.begin() || i + 1 >= (long)stroke.end())
		return util::point<double>(0, 0);

	util::point<double> begin = points[i].position;
	util::point<double> end   = points[i+1].position;

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
