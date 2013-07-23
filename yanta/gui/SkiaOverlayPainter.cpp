#include <SkDashPathEffect.h>

#include <util/Logger.h>
#include "SkiaDocumentPainter.h"
#include "SkiaOverlayPainter.h"

logger::LogChannel skiaoverlaypainterlog("skiaoverlaypainterlog", "[SkiaOverlayPainter] ");

void
SkiaOverlayPainter::draw(
		SkCanvas& canvas,
		const util::rect<DocumentPrecision>& roi) {

	setCanvas(canvas);
	setRoi(roi);

	prepare();

	// clear the surface, respecting the clipping
	canvas.clear(SkColorSetARGB(0, 255, 255, 255));

	{
		// make sure reading access to the stroke points are safe
		boost::shared_lock<boost::shared_mutex> lock(getDocument().getStrokePoints().getMutex());

		LOG_DEBUG(skiaoverlaypainterlog) << "starting to visit document" << std::endl;

		// go visit the document to draw overlay elements
		getDocument().accept(*this);

		LOG_DEBUG(skiaoverlaypainterlog) << "done to visiting document" << std::endl;
	}

	// draw the tools
	for (Tools::iterator i = _tools->begin(); i != _tools->end(); i++) {

		Tool& tool = *(*i);

		if (tool.getBoundingBox().intersects(getRoi()))
			tool.draw(canvas);
	}

	finish();
}

void
SkiaOverlayPainter::visit(Selection& selection) {

	// draw the outline of the selection

	util::rect<PagePrecision> bb = (selection.getBoundingBox() - selection.getShift())/selection.getScale();

	SkPath path;
	path.moveTo(bb.minX, bb.minY);
	path.lineTo(bb.maxX, bb.minY);
	path.lineTo(bb.maxX, bb.maxY);
	path.lineTo(bb.minX, bb.maxY);
	path.lineTo(bb.minX, bb.minY);

	SkPaint paint;
	paint.setColor(SkColorSetARGB(80, 0, 0, 0));
	paint.setAntiAlias(true);
	paint.setStyle(SkPaint::kFill_Style);
	getCanvas().drawPath(path, paint);

	SkScalar intervals[] = {3, 2};
	SkDashPathEffect dash(intervals, 2, 0);
	paint.setStrokeWidth(0.5);
	paint.setColor(SkColorSetARGB(255, 0, 0, 0));
	paint.setPathEffect(&dash);
	paint.setStyle(SkPaint::kStroke_Style);
	getCanvas().drawPath(path, paint);
}
