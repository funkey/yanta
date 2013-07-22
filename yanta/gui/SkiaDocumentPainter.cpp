#include <util/Logger.h>
#include "SkiaDocumentPainter.h"
#include "SkiaStrokePainter.h"

logger::LogChannel skiadocumentpainterlog("skiadocumentpainterlog", "[SkiaDocumentPainter] ");

SkiaDocumentPainter::SkiaDocumentPainter(
		const gui::skia_pixel_t& clearColor,
		bool drawPaper) :
	_clearColor(clearColor),
	_drawPaper(drawPaper),
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

	setCanvas(canvas);

	if (roi.isZero())

		setRoi(roi);

	else {

		// clip outside our responsibility
		canvas.clipRect(SkRect::MakeLTRB(roi.minX, roi.minY, roi.maxX, roi.maxY));

		// transform roi downwards
		setRoi((roi - _pixelOffset)/_pixelsPerDeviceUnit);
	}

	// apply the device transformation
	canvas.save();
	canvas.translate(_pixelOffset.x, _pixelOffset.y);
	canvas.scale(_pixelsPerDeviceUnit.x, _pixelsPerDeviceUnit.y);

	// reset temporal memory about what we drew already
	_drawnUntilStrokePointTmp = _drawnUntilStrokePoint;

	{
		// make sure reading access to the stroke points are safe
		boost::shared_lock<boost::shared_mutex> lock(_document->getStrokePoints().getMutex());

		// go visit the document
		_document->accept(*this);
	}

	// restore original transformation
	canvas.restore();

	LOG_ALL(skiadocumentpainterlog) << "I drew everything until stroke point " << _drawnUntilStrokePointTmp << std::endl;
}

void
SkiaDocumentPainter::enter(DocumentElement& element) {

	const util::point<DocumentPrecision>& shift = element.getShift();
	const util::point<DocumentPrecision>& scale = element.getScale();

	_canvas->save();

	if (!getRoi().isZero()) {

		// transform roi downwards
		util::rect<double> transformedRoi = getRoi();
		transformedRoi -= shift;
		transformedRoi /= scale;
		setRoi(transformedRoi);
	}

	// apply the element transformation
	_canvas->translate(shift.x, shift.y);
	_canvas->scale(scale.x, scale.y);
}

void
SkiaDocumentPainter::leave(DocumentElement& element) {

	const util::point<DocumentPrecision>& shift = element.getShift();
	const util::point<DocumentPrecision>& scale = element.getScale();

	_canvas->restore();

	if (!getRoi().isZero()) {

		// transform roi upwards
		util::rect<DocumentPrecision> roi = getRoi();
		roi *= scale;
		roi += shift;
		setRoi(roi);
	}
}

void
SkiaDocumentPainter::visit(Document&) {

	LOG_DEBUG(skiadocumentpainterlog) << "visiting document" << std::endl;

	// clear the surface, respecting the clipping
	if (!_incremental)
		_canvas->drawColor(SkColorSetRGB(_clearColor.blue, _clearColor.green, _clearColor.red));
}

void
SkiaDocumentPainter::visit(Page& page) {

	LOG_ALL(skiadocumentpainterlog) << "visiting page" << std::endl;

	if (!_incremental && _drawPaper)
		drawPaper(page);
}

void
SkiaDocumentPainter::drawPaper(Page& page) {

	// even though the roi might intersect the page's content, it might not 
	// intersect the paper -- check that here (in page coordinates)
	if (!getRoi().isZero() && !getRoi().intersects(util::rect<PagePrecision>(0, 0, page.getSize().x, page.getSize().y)))
		return;

	SkPaint paint;
	paint.setStrokeCap(SkPaint::kRound_Cap);
	paint.setColor(SkColorSetRGB(0, 0, 0));
	paint.setAntiAlias(true);
	const util::point<PagePrecision>& pageSize = page.getSize();
	_canvas->drawLine(0, 0, pageSize.x, 0, paint);
	_canvas->drawLine(pageSize.x, 0, pageSize.x, pageSize.y, paint);
	_canvas->drawLine(pageSize.x, pageSize.y, 0, pageSize.y, paint);
	_canvas->drawLine(0, pageSize.y, 0, 0, paint);
	paint.setColor(SkColorSetRGB(0, 203, 0));
	paint.setStrokeWidth(0.01);

	double startX = std::max(getRoi().minX, 0.0);
	double startY = std::max(getRoi().minY, 0.0);
	double endX   = getRoi().isZero() ? pageSize.x : std::min(getRoi().maxX, pageSize.x);
	double endY   = getRoi().isZero() ? pageSize.y : std::min(getRoi().maxY, pageSize.y);

	for (int x = (int)startX; x < (int)endX; x++)
		_canvas->drawLine(x, startY, x, endY, paint);
	for (int y = (int)startY; y < (int)endY; y++) {
		if (y%10 == 0)
			paint.setStrokeWidth(0.05);
		_canvas->drawLine(startX, y, endX, y, paint);
		if (y%10 == 0)
			paint.setStrokeWidth(0.01);
	}
}

void
SkiaDocumentPainter::visit(Stroke& stroke) {

	// the effective end of a stroke is one less, if the stroke is not finished, 
	// yet
	long begin = std::max((long)_drawnUntilStrokePoint - 1, (long)stroke.begin());
	long end   = (long)stroke.end() - (stroke.finished() ? 0 : 1);

	// drawn already?
	if (end <= (long)_drawnUntilStrokePoint)
		return;

	// remember the biggest point we drew already
	_drawnUntilStrokePointTmp = std::max(_drawnUntilStrokePointTmp, (unsigned long)end);

	SkiaStrokePainter strokePainter(*_canvas, _document->getStrokePoints());

	LOG_ALL(skiadocumentpainterlog)
			<< "drawing stroke (" << stroke.begin() << " - " << stroke.end()
			<< ") , starting from point " << begin << " until " << end << std::endl;

	strokePainter.draw(stroke, getRoi(), begin, end);
}

#if 0
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
#endif
