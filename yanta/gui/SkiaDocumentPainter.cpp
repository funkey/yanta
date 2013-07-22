#include <util/Logger.h>
#include "SkiaDocumentPainter.h"
#include "SkiaStrokePainter.h"

logger::LogChannel skiadocumentpainterlog("skiadocumentpainterlog", "[SkiaDocumentPainter] ");

SkiaDocumentPainter::SkiaDocumentPainter(
		const gui::skia_pixel_t& clearColor,
		bool drawPaper) :
	_clearColor(clearColor),
	_drawPaper(drawPaper),
	_drawnUntilStrokePoint(0),
	_drawnUntilStrokePointTmp(0) {}

void
SkiaDocumentPainter::draw(SkCanvas& canvas, const util::rect<DocumentPrecision>& roi) {

	setCanvas(canvas);
	setRoi(roi);

	prepare();

	{
		// make sure reading access to the stroke points are safe
		boost::shared_lock<boost::shared_mutex> lock(getDocument().getStrokePoints().getMutex());

		// go visit the document
		getDocument().accept(*this);
	}

	finish();
}

bool
SkiaDocumentPainter::alreadyDrawn(const Document& document) {

	// is it necessary to draw something?
	if (document.numStrokes() > 0 && _drawnUntilStrokePoint == document.getStrokePoints().size() - (document.hasOpenStroke() ? 0 : 1))
		return true;

	return false;
}

void
SkiaDocumentPainter::visit(Document&) {

	LOG_DEBUG(skiadocumentpainterlog) << "visiting document" << std::endl;

	// reset temporal memory about what we drew already
	_drawnUntilStrokePointTmp = _drawnUntilStrokePoint;

	// clear the surface, respecting the clipping
	if (!_incremental)
		getCanvas().drawColor(SkColorSetRGB(_clearColor.blue, _clearColor.green, _clearColor.red));
}

void
SkiaDocumentPainter::visit(Page& page) {

	LOG_ALL(skiadocumentpainterlog) << "visiting page" << std::endl;

	if (_incremental || !_drawPaper)
		return;

	// even though the roi might intersect the page's content, it might not 
	// intersect the paper -- check that here (in page coordinates)
	if (!getRoi().isZero() && !getRoi().intersects(util::rect<PagePrecision>(0, 0, page.getSize().x, page.getSize().y)))
		return;

	SkPaint paint;
	paint.setStrokeCap(SkPaint::kRound_Cap);
	paint.setColor(SkColorSetRGB(0, 0, 0));
	paint.setAntiAlias(true);
	const util::point<PagePrecision>& pageSize = page.getSize();
	getCanvas().drawLine(0, 0, pageSize.x, 0, paint);
	getCanvas().drawLine(pageSize.x, 0, pageSize.x, pageSize.y, paint);
	getCanvas().drawLine(pageSize.x, pageSize.y, 0, pageSize.y, paint);
	getCanvas().drawLine(0, pageSize.y, 0, 0, paint);
	paint.setColor(SkColorSetRGB(0, 203, 0));
	paint.setStrokeWidth(0.01);

	double startX = std::max(getRoi().minX, 0.0);
	double startY = std::max(getRoi().minY, 0.0);
	double endX   = getRoi().isZero() ? pageSize.x : std::min(getRoi().maxX, pageSize.x);
	double endY   = getRoi().isZero() ? pageSize.y : std::min(getRoi().maxY, pageSize.y);

	for (int x = (int)startX; x < (int)endX; x++)
		getCanvas().drawLine(x, startY, x, endY, paint);
	for (int y = (int)startY; y < (int)endY; y++) {
		if (y%10 == 0)
			paint.setStrokeWidth(0.05);
		getCanvas().drawLine(startX, y, endX, y, paint);
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

	SkiaStrokePainter strokePainter(getCanvas(), getDocument().getStrokePoints());

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
