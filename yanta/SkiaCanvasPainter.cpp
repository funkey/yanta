#include <util/Logger.h>
#include "SkiaCanvasPainter.h"

logger::LogChannel skiabackendpainterlog("skiabackendpainterlog", "[SkiaCanvasPainter] ");

SkiaCanvasPainter::SkiaCanvasPainter(const gui::skia_pixel_t& clearColor) :
	_clearColor(clearColor),
	_pixelsPerDeviceUnit(1.0, 1.0),
	_pixelOffset(0, 0),
	_drawnUntilStrokePoint(0),
	_drawnUntilStrokePointTmp(0) {}

bool
SkiaCanvasPainter::alreadyDrawn(const Canvas& canvas) {

	// is it necessary to draw something?
	if (canvas.numStrokes() > 0 && _drawnUntilStrokePoint == canvas.getStrokePoints().size() - (canvas.hasOpenStroke() ? 0 : 1))
		return true;

	return false;
}

void
SkiaCanvasPainter::draw(SkCanvas& canvas) {

	util::rect<CanvasPrecision> roi(0, 0, 0, 0);
	draw(canvas, roi);
}

void
SkiaCanvasPainter::draw(
		SkCanvas& canvas,
		const util::rect<CanvasPrecision>& roi) {

	bool incremental = (_drawnUntilStrokePoint != 0);

	LOG_ALL(skiabackendpainterlog) << "drawing " << (incremental ? "" : "non-") << "incrementally" << std::endl;

	canvas.save();

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

	// prepare the background, if we do not draw incrementally
	if (!incremental) {

		clearSurface(canvas);
		drawPaper(canvas, canvasRoi);
	}

	// make sure reading access to the stroke points are safe
	boost::shared_lock<boost::shared_mutex> lock(_canvas->getStrokePoints().getMutex());

	// reset temporal memory about what we drew already
	_drawnUntilStrokePointTmp = _drawnUntilStrokePoint;

	// for every page...
	for (unsigned int i = 0; i < _canvas->numPages(); i++) {

		const Page& page = _canvas->getPage(i);

		const util::point<CanvasPrecision> pagePosition = page.getPosition();

		// correct for the page position...
		canvas.save();
		canvas.translate(pagePosition.x, pagePosition.y); // TODO: precision-problematic conversion
		util::rect<double> pageRoi = canvasRoi - pagePosition; // TODO: precision-problematic conversion

		drawPage(canvas, page, pageRoi);

		// page offset
		canvas.restore();
	}

	// canvas to pixels
	canvas.restore();

	LOG_ALL(skiabackendpainterlog) << "I drew everything until stroke point " << _drawnUntilStrokePointTmp << std::endl;
}

void
SkiaCanvasPainter::clearSurface(SkCanvas& canvas) {

	// clear the surface, respecting the clipping
	canvas.drawColor(SkColorSetRGB(_clearColor.blue, _clearColor.green, _clearColor.red));
}

void
SkiaCanvasPainter::drawPaper(SkCanvas& canvas, const util::rect<double>& canvasRoi) {

	// draw pages
	for (unsigned int i = 0; i < _canvas->numPages(); i++) {

		const Page& page = _canvas->getPage(i);

		if (!canvasRoi.isZero() && !canvasRoi.intersects(page.getPageBoundingBox()))
			continue;

		const util::point<CanvasPrecision> pagePosition = page.getPosition();

		// correct for the page position...
		canvas.save();
		canvas.translate(pagePosition.x, pagePosition.y); // TODO: precision-problematic conversion

		SkPaint paint;
		paint.setStrokeCap(SkPaint::kRound_Cap);
		paint.setColor(SkColorSetRGB(0, 0, 0));
		paint.setAntiAlias(true);
		const util::point<PagePrecision>& pageSize = page.getSize();
		canvas.drawLine(0, 0, pageSize.x, 0, paint);
		canvas.drawLine(pageSize.x, 0, pageSize.x, pageSize.y, paint);
		canvas.drawLine(pageSize.x, pageSize.y, 0, pageSize.y, paint);
		canvas.drawLine(0, pageSize.y, 0, 0, paint);
		paint.setColor(SkColorSetRGB(0, 203, 0));
		paint.setStrokeWidth(0.01);
		for (int x = 0; x < (int)pageSize.x; x++)
			canvas.drawLine(x, 0, x, pageSize.y, paint);
		for (int y = 0; y < (int)pageSize.y; y++) {
			if (y%10 == 0)
				paint.setStrokeWidth(0.05);
			canvas.drawLine(0, y, pageSize.x, y, paint);
			if (y%10 == 0)
				paint.setStrokeWidth(0.01);
		}

		canvas.restore();
	}
}

void
SkiaCanvasPainter::drawPage(SkCanvas& canvas, const Page& page, const util::rect<double>& pageRoi) {

	// draw every stroke
	for (unsigned int i = 0; i < page.numStrokes(); i++) {

		const Stroke& stroke = page.getStroke(i);

		// the effective end of a stroke is one less, if the stroke is not 
		// finished, yet
		long begin = std::max((long)_drawnUntilStrokePoint - 1, (long)stroke.begin());
		long end   = (long)stroke.end() - (stroke.finished() ? 0 : 1);

		// drawn already?
		if (end <= (long)_drawnUntilStrokePoint)
			continue;

		// remember the biggest point we drew already
		_drawnUntilStrokePointTmp = std::max(_drawnUntilStrokePointTmp, (unsigned long)end);

		// draw only of no roi given or stroke intersects roi
		if (pageRoi.isZero() || !stroke.finished() || stroke.boundingBox().intersects(pageRoi)) {

			LOG_ALL(skiabackendpainterlog)
					<< "drawing stroke " << i << " (" << stroke.begin() << " - " << stroke.end()
					<< ") , starting from point " << begin << " until " << end << std::endl;

			drawStroke(canvas, stroke, pageRoi, begin, end);

		} else {

			LOG_ALL(skiabackendpainterlog) << "stroke " << i << " is not visible" << std::endl;
		}
	}
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

		LOG_ALL(skiabackendpainterlog) << "this stroke has less than two points -- skip drawing" << std::endl;
		return false;
	}

	canvas.save();
	canvas.translate(stroke.getShift().x, stroke.getShift().y);
	canvas.scale(stroke.getScale().x, stroke.getScale().y);

	double penWidth = stroke.getStyle().width();
	unsigned char penColorRed   = stroke.getStyle().getRed();
	unsigned char penColorGreen = stroke.getStyle().getGreen();
	unsigned char penColorBlue  = stroke.getStyle().getBlue();

	const StrokePoints& strokePoints = _canvas->getStrokePoints();

	SkPaint paint;
	paint.setStrokeCap(SkPaint::kRound_Cap);
	paint.setColor(SkColorSetRGB(penColorRed, penColorGreen, penColorBlue));
	paint.setAntiAlias(true);

	// for each line in the stroke
	for (unsigned long i = beginStroke; i < endStroke - 1; i++) {

		//double alpha = alphaPressureCurve(stroke[i].pressure);
		double width = widthPressureCurve(strokePoints[i].pressure);

		paint.setStrokeWidth(width*penWidth);

		canvas.drawLine(
				strokePoints[i  ].position.x, strokePoints[i  ].position.y,
				strokePoints[i+1].position.x, strokePoints[i+1].position.y,
				paint);
	}

	canvas.restore();

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

	const double minPressure = 0.5;
	const double maxPressure = 1;

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
