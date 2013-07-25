#include <SkMaskFilter.h>
#include <SkBlurMaskFilter.h>

#include <util/Logger.h>
#include "SkiaDocumentPainter.h"

logger::LogChannel skiadocumentpainterlog("skiadocumentpainterlog", "[SkiaDocumentPainter] ");

SkiaDocumentPainter::SkiaDocumentPainter(
		const gui::skia_pixel_t& clearColor,
		bool drawPaper) :
	_clearColor(clearColor),
	_drawPaper(drawPaper),
	_drawnUntilStrokePoint(0),
	_drawnUntilStrokePointTmp(0),
	_canvasCleared(false),
	_canvasClearedTmp(false),
	_paperDrawn(false),
	_paperDrawnTmp(false) {}

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
SkiaDocumentPainter::needRedraw() {

	if (!_canvasCleared || !_paperDrawn)
		return true;

	if (getDocument().numStrokes() == 0)
		return false;

	// did we draw all the stroke points?
	if (_drawnUntilStrokePoint == getDocument().getStrokePoints().size())
		return false;

	return true;
}

void
SkiaDocumentPainter::visit(Document&) {

	LOG_DEBUG(skiadocumentpainterlog) << "visiting document" << std::endl;

	// reset temporal memory about what we drew already
	_drawnUntilStrokePointTmp = _drawnUntilStrokePoint;
	_canvasClearedTmp = _canvasCleared;
	_paperDrawnTmp = _paperDrawn;

	// clear the surface, respecting the clipping
	if (!_incremental || !_canvasCleared) {
		getCanvas().drawColor(SkColorSetRGB(_clearColor.blue, _clearColor.green, _clearColor.red));
		_canvasClearedTmp = true;
	}
}

void
SkiaDocumentPainter::visit(Page& page) {

	LOG_ALL(skiadocumentpainterlog) << "visiting page" << std::endl;

	if ((_incremental && _paperDrawn) || !_drawPaper)
		return;

	// even though the roi might intersect the page's content, it might not 
	// intersect the paper -- check that here (in page coordinates)
	if (!getRoi().isZero() && !getRoi().intersects(util::rect<PagePrecision>(0, 0, page.getSize().x, page.getSize().y)))
		return;

	SkPath outline;
	const util::point<PagePrecision>& pageSize = page.getSize();
	outline.lineTo(pageSize.x, 0);
	outline.lineTo(pageSize.x, pageSize.y);
	outline.lineTo(0, pageSize.y);
	outline.lineTo(0, 0);
	outline.close();

	const char pageRed   = 255;
	const char pageGreen = 255;
	const char pageBlue  = 245;

	const char gridRed   = 55;
	const char gridGreen = 55;
	const char gridBlue  = 45;

	const double gridSizeX = 5.0;
	const double gridSizeY = 5.0;
	const double gridWidth = 0.03;

	SkPaint paint;

	// shadow-like thingie

	SkMaskFilter* maskFilter = SkBlurMaskFilter::Create(5, SkBlurMaskFilter::kOuter_BlurStyle, SkBlurMaskFilter::kNone_BlurFlag);
	paint.setMaskFilter(maskFilter)->unref();
	paint.setColor(SkColorSetRGB(0.5*pageRed, 0.5*pageGreen, 0.5*pageBlue));
	getCanvas().drawPath(outline, paint);
	paint.setMaskFilter(NULL);

	// the paper

	paint.setStyle(SkPaint::kFill_Style);
	paint.setColor(SkColorSetRGB(pageRed, pageGreen, pageBlue));
	getCanvas().drawPath(outline, paint);

	// the grid

	paint.setStyle(SkPaint::kStroke_Style);
	paint.setColor(SkColorSetRGB(gridRed, gridGreen, gridBlue));
	paint.setStrokeWidth(gridWidth);
	paint.setAntiAlias(true);

	double startX = std::max(getRoi().minX, 0.0);
	double startY = std::max(getRoi().minY, 0.0);
	double endX   = getRoi().isZero() ? pageSize.x : std::min(getRoi().maxX, pageSize.x);
	double endY   = getRoi().isZero() ? pageSize.y : std::min(getRoi().maxY, pageSize.y);

	for (int x = (int)ceil(startX/gridSizeX)*gridSizeX; x <= (int)floor(endX/gridSizeX)*gridSizeX; x += gridSizeX)
		getCanvas().drawLine(x, startY, x, endY, paint);
	for (int y = (int)ceil(startY/gridSizeY)*gridSizeY; y <= (int)floor(endY/gridSizeY)*gridSizeY; y += gridSizeY)
		getCanvas().drawLine(startX, y, endX, y, paint);

	// the outline

	paint.setColor(SkColorSetRGB(0.5*pageRed, 0.5*pageGreen, 0.5*pageBlue));
	paint.setStrokeWidth(gridWidth);
	paint.setStrokeCap(SkPaint::kRound_Cap);
	paint.setStrokeJoin(SkPaint::kRound_Join);
	getCanvas().drawPath(outline, paint);

	_paperDrawnTmp = true;
}

void
SkiaDocumentPainter::visit(Stroke& stroke) {

	unsigned long end = stroke.end();

	// end is one beyond the last point of the stroke. _drawnUntilStrokePoint is 
	// one beyond the last point until which we drew already.  If end is less or 
	// equal what we drew, there is nothing to do.

	// drawn already?
	if (_incremental && end <= _drawnUntilStrokePoint)
		return;

	unsigned long begin = stroke.begin();

	if (_incremental && _drawnUntilStrokePoint > 0 && _drawnUntilStrokePoint - 1 > stroke.begin())
		begin = _drawnUntilStrokePoint - 1;

	LOG_ALL(skiadocumentpainterlog)
			<< "drawing stroke (" << stroke.begin() << " - " << stroke.end()
			<< ") , starting from point " << begin << " until " << end << std::endl;

	_strokePainter.draw(getCanvas(), getDocument().getStrokePoints(), stroke, getRoi(), begin, end);

	// remember until which point we drew already in our temporary memory
	_drawnUntilStrokePointTmp = std::max(_drawnUntilStrokePointTmp, end);
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
