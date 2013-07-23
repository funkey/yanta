#include "Lasso.h"

void
Lasso::draw(SkCanvas& canvas) {

	SkPaint paint;
	paint.setColor(SkColorSetARGB(80, 0, 0, 0));
	paint.setAntiAlias(true);
	paint.setStrokeWidth(2.0);

	canvas.drawPath(_path, paint);
}
