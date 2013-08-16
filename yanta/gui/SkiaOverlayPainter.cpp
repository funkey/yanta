#include <SkMaskFilter.h>
#include <SkBlurMaskFilter.h>

#include <util/Logger.h>
#include "SkiaDocumentPainter.h"
#include "SkiaOverlayPainter.h"

logger::LogChannel skiaoverlaypainterlog("skiaoverlaypainterlog", "[SkiaOverlayPainter] ");

SkiaOverlayPainter::SkiaOverlayPainter() {

	_selectionPaint.setColor(SkColorSetARGB(25, 0, 0, 0));
	_selectionPaint.setAntiAlias(true);
	_selectionPaint.setStyle(SkPaint::kFill_Style);
	SkMaskFilter* maskFilter = SkBlurMaskFilter::Create(0.1, SkBlurMaskFilter::kNormal_BlurStyle);
	_selectionPaint.setMaskFilter(maskFilter)->unref();
}

void
SkiaOverlayPainter::draw(
		SkCanvas& canvas,
		const util::rect<DocumentPrecision>& roi) {

	setCanvas(canvas);

	prepare(roi);

	// clear the surface, respecting the clipping
	canvas.clear(SkColorSetARGB(0, 255, 255, 255));

	{
		// make sure reading access to the stroke points are safe
		boost::shared_lock<boost::shared_mutex> lock(getDocument().getStrokePoints().getMutex());

		LOG_DEBUG(skiaoverlaypainterlog) << "starting to visit document" << std::endl;

		// go visit the document to draw overlay elements
		getDocument().accept(*this);

		LOG_DEBUG(skiaoverlaypainterlog) << "done visiting document" << std::endl;
	}

	// draw the tools
	for (Tools::iterator i = _tools->begin(); i != _tools->end(); i++) {

		LOG_ALL(skiaoverlaypainterlog) << "drawing a tool" << std::endl;

		Tool& tool = *(*i);

		if (tool.getBoundingBox().intersects(getRoi()))
			tool.draw(canvas);
	}

	finish();
}

void
SkiaOverlayPainter::visit(Selection& selection) {

	LOG_ALL(skiaoverlaypainterlog) << "found a selection at " << selection.getBoundingBox() << std::endl;

	// draw the outline of the selection

	util::rect<PagePrecision> bb = (selection.getBoundingBox() - selection.getShift())/selection.getScale();

	SkPath path;
	path.moveTo(bb.minX, bb.minY);
	path.lineTo(bb.maxX, bb.minY);
	path.lineTo(bb.maxX, bb.maxY);
	path.lineTo(bb.minX, bb.maxY);
	path.lineTo(bb.minX, bb.minY);

	getCanvas().drawPath(path, _selectionPaint);
}
