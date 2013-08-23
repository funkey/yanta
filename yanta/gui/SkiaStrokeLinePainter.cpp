#include <SkCanvas.h>

#include <document/Stroke.h>
#include <document/StrokePoints.h>
#include "SkiaStrokeLinePainter.h"
#include "util/Logger.h"

void
SkiaStrokeLinePainter::draw(
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

	unsigned int step = 1;

	if (getQuality() <= Worst)
		step = 9;
	else if (getQuality() <= Medium)
		step = 3;

	// for each line in the stroke
	for (unsigned long i = beginStroke; i < endStroke - 1; i += step) {

		//double alpha = alphaPressureCurve(stroke[i].pressure);
		double width = widthPressureCurve(strokePoints[i].pressure);

		paint.setStrokeWidth(width*penWidth);

		unsigned int next = std::min(i + step, endStroke - 1);

		canvas.drawLine(
				strokePoints[i   ].position.x, strokePoints[i   ].position.y,
				strokePoints[next].position.x, strokePoints[next].position.y,
				paint);
	}

	return;
}

double
SkiaStrokeLinePainter::widthPressureCurve(double pressure) {

	const double minPressure = 0.5;
	const double maxPressure = 1;

	pressure /= 2048.0;

	return minPressure + pressure*(maxPressure - minPressure);
}

double
SkiaStrokeLinePainter::alphaPressureCurve(double pressure) {

	const double minPressure = 1;
	const double maxPressure = 1;

	pressure /= 2048.0;

	return minPressure + pressure*(maxPressure - minPressure);
}
