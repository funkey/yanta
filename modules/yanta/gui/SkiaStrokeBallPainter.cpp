#include <SkCanvas.h>
#include <SkMaskFilter.h>
#include <SkBlurMaskFilter.h>

#include <document/Stroke.h>
#include <document/StrokePoints.h>
#include "SkiaStrokeBallPainter.h"
#include "util/Logger.h"

SkiaStrokeBallPainter::SkiaStrokeBallPainter() {}

void
SkiaStrokeBallPainter::draw(
		SkCanvas& canvas,
		const StrokePoints& strokePoints,
		const Stroke& stroke,
		const util::rect<double>& /*roi*/,
		unsigned long beginStroke,
		unsigned long endStroke) {

	if (beginStroke == 0 && endStroke == 0) {

		beginStroke = stroke.begin();
		endStroke   = stroke.end();
	}

	// make sure there are enough points in this stroke to draw it
	if (endStroke - beginStroke == 0)
		return;

	double penWidth = stroke.getStyle().width();
	unsigned char penColorRed   = stroke.getStyle().getRed();
	unsigned char penColorGreen = stroke.getStyle().getGreen();
	unsigned char penColorBlue  = stroke.getStyle().getBlue();

	SkPaint paint;
	paint.setColor(SkColorSetRGB(penColorRed, penColorGreen, penColorBlue));
	paint.setAntiAlias(true);

	SkMaskFilter* maskFilter = SkBlurMaskFilter::Create(0.05*penWidth, SkBlurMaskFilter::kNormal_BlurStyle);
	paint.setMaskFilter(maskFilter)->unref();

	util::point<PagePrecision> previousPosition = strokePoints[beginStroke].position;
	double pos = 0;
	double length = 0;
	const double step = 0.1*penWidth;

	// if we start drawing in the middle of the stroke, we need to get the 
	// length of the stroke until our beginning
	if (stroke.begin() < beginStroke) {

		for (unsigned long i = stroke.begin() + 1; i <= beginStroke; i++) {
			util::point<PagePrecision> diff = strokePoints[i].position - strokePoints[i-1].position;
			length += sqrt(diff.x*diff.x + diff.y*diff.y);
		}

		pos = length - fmod(length, step) + step;
	}

	// for each line in the stroke
	for (unsigned long i = beginStroke + 1; i < endStroke; i++) {

		const util::point<PagePrecision>& nextPosition = strokePoints[i].position;

		util::point<PagePrecision> diff = nextPosition - previousPosition;

		double lineLength = sqrt(diff.x*diff.x + diff.y*diff.y);

		for (; pos <= length + lineLength; pos += step) {

			double a = (pos - length)/lineLength;

			util::point<PagePrecision> p = previousPosition + a*diff;
			double pressure = (1-a)*strokePoints[i-1].pressure + a*strokePoints[i].pressure;

			double alpha = alphaPressureCurve(pressure);
			double width = widthPressureCurve(pressure);

			paint.setAlpha(alpha*255.0);

			canvas.drawCircle(p.x, p.y, 0.5*width*penWidth, paint);
		}

		length += lineLength;
		previousPosition = nextPosition;
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
