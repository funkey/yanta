#include <SkPath.h>
#include <SkDashPathEffect.h>
#include "Lasso.h"
#include "Selection.h"
#include "SkiaOverlayObjectPainter.h"
#include "SkiaStrokePainter.h"

void
SkiaOverlayObjectPainter::processLasso(const Lasso& lasso) {

	const SkPath& path = lasso.getPath();
	SkPaint paint;
	paint.setColor(SkColorSetARGB(80, 0, 0, 0));
	paint.setAntiAlias(true);
	paint.setStrokeWidth(2.0);

	_canvas.drawPath(path, paint);
}

void
SkiaOverlayObjectPainter::processSelection(const Selection& selection) {

	// the strokes in the selection

	_canvas.save();

	_canvas.translate(selection.getShift().x, selection.getShift().y);
	_canvas.scale(selection.getScale().x, selection.getScale().y);

	SkiaStrokePainter strokePainter(_canvas, selection.getStrokePoints());

	for (unsigned int i = 0; i < selection.numStrokes(); i++)
		strokePainter.draw(selection.getStroke(i), util::rect<CanvasPrecision>(0, 0, 0, 0));

	_canvas.restore();

	// the selection rectangle

	SkPath path;
	path.moveTo(selection.getBoundingBox().minX, selection.getBoundingBox().minY);
	path.lineTo(selection.getBoundingBox().maxX, selection.getBoundingBox().minY);
	path.lineTo(selection.getBoundingBox().maxX, selection.getBoundingBox().maxY);
	path.lineTo(selection.getBoundingBox().minX, selection.getBoundingBox().maxY);
	path.lineTo(selection.getBoundingBox().minX, selection.getBoundingBox().minY);

	SkPaint paint;
	paint.setColor(SkColorSetARGB(80, 0, 0, 0));
	paint.setAntiAlias(true);
	paint.setStyle(SkPaint::kFill_Style);
	_canvas.drawPath(path, paint);

	SkScalar intervals[] = {3, 2};
	SkDashPathEffect dash(intervals, 2, 0);
	paint.setStrokeWidth(0.5);
	paint.setColor(SkColorSetARGB(255, 0, 0, 0));
	paint.setPathEffect(&dash);
	paint.setStyle(SkPaint::kStroke_Style);
	_canvas.drawPath(path, paint);
}
