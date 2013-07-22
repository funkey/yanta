#include <util/Logger.h>
#include "SkiaDocumentPainter.h"
#include "SkiaStrokePainter.h"

logger::LogChannel skiabackendpainterlog("skiabackendpainterlog", "[SkiaDocumentPainter] ");

SkiaDocumentPainter::SkiaDocumentPainter(const gui::skia_pixel_t& clearColor) :
	_clearColor(clearColor),
	_pixelsPerDeviceUnit(1.0, 1.0),
	_pixelOffset(0, 0),
	_drawnUntilStrokePoint(0),
	_drawnUntilStrokePointTmp(0) {}

bool
SkiaDocumentPainter::alreadyDrawn(const Document& document) {

	// is it necessary to draw something?
	if (document.numStrokes() > 0 && _drawnUntilStrokePoint == document.getStrokePoints().size() - (document.hasOpenStroke() ? 0 : 1))
		return true;

	return false;
}

void
SkiaDocumentPainter::draw(SkCanvas& canvas) {

	util::rect<DocumentPrecision> roi(0, 0, 0, 0);
	draw(canvas, roi);
}

void
SkiaDocumentPainter::draw(
		SkCanvas& canvas,
		const util::rect<DocumentPrecision>& roi) {

	bool incremental = (_drawnUntilStrokePoint != 0);

	LOG_ALL(skiabackendpainterlog) << "drawing " << (incremental ? "" : "non-") << "incrementally" << std::endl;

	canvas.save();

	util::rect<double> documentRoi(0, 0, 0, 0);

	if (!roi.isZero()) {

		// clip outside our responsibility
		canvas.clipRect(SkRect::MakeLTRB(roi.minX, roi.minY, roi.maxX, roi.maxY));

		// transform roi to canvas units
		documentRoi = roi;
		documentRoi -= _pixelOffset; // TODO: precision-problematic conversion
		documentRoi /= _pixelsPerDeviceUnit;
	}

	// apply the given transformation

	// translate should be performed...
	canvas.translate(_pixelOffset.x, _pixelOffset.y); // TODO: precision-problematic conversion
	// ...after the scaling
	canvas.scale(_pixelsPerDeviceUnit.x, _pixelsPerDeviceUnit.y);

	// prepare the background, if we do not draw incrementally
	if (!incremental) {

		clearSurface(canvas);
		drawPaper(canvas, documentRoi);
	}

	// make sure reading access to the stroke points are safe
	boost::shared_lock<boost::shared_mutex> lock(_document->getStrokePoints().getMutex());

	// reset temporal memory about what we drew already
	_drawnUntilStrokePointTmp = _drawnUntilStrokePoint;

	// for every page...
	for (unsigned int i = 0; i < _document->numPages(); i++) {

		const Page& page = _document->getPage(i);

		const util::point<DocumentPrecision> pagePosition = page.getPosition();

		// correct for the page position...
		canvas.save();
		canvas.translate(pagePosition.x, pagePosition.y); // TODO: precision-problematic conversion
		util::rect<double> pageRoi = documentRoi - pagePosition; // TODO: precision-problematic conversion

		drawPage(canvas, page, pageRoi);

		// page offset
		canvas.restore();
	}

	// canvas to pixels
	canvas.restore();

	LOG_ALL(skiabackendpainterlog) << "I drew everything until stroke point " << _drawnUntilStrokePointTmp << std::endl;
}

void
SkiaDocumentPainter::clearSurface(SkCanvas& canvas) {

	// clear the surface, respecting the clipping
	canvas.drawColor(SkColorSetRGB(_clearColor.blue, _clearColor.green, _clearColor.red));
}

void
SkiaDocumentPainter::drawPaper(SkCanvas& canvas, const util::rect<double>& documentRoi) {

	// draw pages
	for (unsigned int i = 0; i < _document->numPages(); i++) {

		const Page& page = _document->getPage(i);

		if (!documentRoi.isZero() && !documentRoi.intersects(page.getPageBoundingBox()))
			continue;

		const util::point<DocumentPrecision> pagePosition = page.getPosition();

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
SkiaDocumentPainter::drawPage(SkCanvas& canvas, const Page& page, const util::rect<double>& pageRoi) {

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
		SkiaStrokePainter strokePainter(canvas, _document->getStrokePoints());
		if (pageRoi.isZero() || !stroke.finished() || stroke.getBoundingBox().intersects(pageRoi)) {

			LOG_ALL(skiabackendpainterlog)
					<< "drawing stroke " << i << " (" << stroke.begin() << " - " << stroke.end()
					<< ") , starting from point " << begin << " until " << end << std::endl;

			strokePainter.draw(stroke, pageRoi, begin, end);

		} else {

			LOG_ALL(skiabackendpainterlog) << "stroke " << i << " is not visible" << std::endl;
		}
	}
}

util::point<double>
SkiaDocumentPainter::getLineNormal(const Stroke& stroke, const StrokePoints& points, long i, double& length) {

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
