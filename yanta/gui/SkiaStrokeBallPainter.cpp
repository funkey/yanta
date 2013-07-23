#include <SkCanvas.h>
#include <SkMaskFilter.h>
#include <SkBlurMaskFilter.h>

#include <document/Stroke.h>
#include <document/StrokePoints.h>
#include "SkiaStrokeBallPainter.h"
#include "util/Logger.h"

SkiaStrokeBallPainter::SkiaStrokeBallPainter(SkCanvas& canvas, const StrokePoints& strokePoints) :
	_canvas(canvas),
	_strokePoints(strokePoints) {}

void
SkiaStrokeBallPainter::draw(
		const Stroke& stroke,
		const util::rect<double>& /*roi*/,
		unsigned long beginStroke,
		unsigned long endStroke) {

	if (beginStroke == 0 && endStroke == 0) {

		beginStroke = stroke.begin();
		endStroke   = stroke.end();
	}

	// make sure there are enough points in this stroke to draw it
	if (stroke.end() - stroke.begin() <= 1)
		return;

	double penWidth = stroke.getStyle().width();
	unsigned char penColorRed   = stroke.getStyle().getRed();
	unsigned char penColorGreen = stroke.getStyle().getGreen();
	unsigned char penColorBlue  = stroke.getStyle().getBlue();

	SkPaint paint;
	paint.setStrokeCap(SkPaint::kRound_Cap);
	paint.setColor(SkColorSetRGB(penColorRed, penColorGreen, penColorBlue));
	paint.setAntiAlias(true);

	SkMaskFilter* maskFilter = SkBlurMaskFilter::Create(0.5, SkBlurMaskFilter::kNormal_BlurStyle);
	paint.setMaskFilter(maskFilter)->unref();

	// for each line in the stroke
	for (unsigned long i = beginStroke; i < endStroke - 1; i++) {

		double alpha = alphaPressureCurve(_strokePoints[i].pressure);
		double width = widthPressureCurve(_strokePoints[i].pressure);

		paint.setAlpha(alpha*255.0);
		paint.setStrokeWidth(width*penWidth);

		_canvas.drawCircle(_strokePoints[i].position.x, _strokePoints[i].position.y, width*penWidth, paint);
	}

	return;
}

double
SkiaStrokeBallPainter::widthPressureCurve(double pressure) {

	const double minPressure = 0.5;
	const double maxPressure = 1;

	pressure /= 2048.0;

	return minPressure + pressure*(maxPressure - minPressure);
}

double
SkiaStrokeBallPainter::alphaPressureCurve(double pressure) {

	const double minPressure = 0;
	const double maxPressure = 1;

	pressure /= 2048.0;

	return minPressure + pressure*(maxPressure - minPressure);
}
