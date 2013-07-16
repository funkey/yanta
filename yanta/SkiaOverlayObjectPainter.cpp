#include <SkPath.h>
#include "Lasso.h"
#include "Selection.h"
#include "SkiaOverlayObjectPainter.h"

void
SkiaOverlayObjectPainter::processLasso(const Lasso& lasso) {

	const SkPath& path = lasso.getPath();
	SkPaint paint;
	paint.setColor(SkColorSetARGB(127, 0, 0, 0));
	paint.setAntiAlias(true);
	paint.setStrokeWidth(2.0);

	_canvas.drawPath(path, paint);
}

void
SkiaOverlayObjectPainter::processSelection(const Selection& selection) {

	SkPath path;
	path.moveTo(selection.getBoundingBox().minX, selection.getBoundingBox().minY);
	path.lineTo(selection.getBoundingBox().maxX, selection.getBoundingBox().minY);
	path.lineTo(selection.getBoundingBox().maxX, selection.getBoundingBox().maxY);
	path.lineTo(selection.getBoundingBox().minX, selection.getBoundingBox().maxY);
	path.lineTo(selection.getBoundingBox().minX, selection.getBoundingBox().minY);

	SkPaint paint;
	paint.setColor(SkColorSetARGB(127, 0, 0, 0));
	paint.setAntiAlias(true);
	paint.setStrokeWidth(2.0);

	_canvas.drawPath(path, paint);
}
