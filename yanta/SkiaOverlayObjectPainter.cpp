#include <SkPath.h>
#include "Lasso.h"
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
