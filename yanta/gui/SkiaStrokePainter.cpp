#include <SkCanvas.h>

#include <document/Stroke.h>
#include <document/StrokePoints.h>
#include "SkiaStrokePainter.h"
#include "util/Logger.h"

logger::LogChannel skiastrokepainterlog("skiastrokepainterlog", "[SkiaStrokePainter] ");

SkiaStrokePainter::SkiaStrokePainter(SkCanvas& canvas, const StrokePoints& strokePoints) :
	_canvas(canvas),
	_strokePoints(strokePoints) {}

void
SkiaStrokePainter::draw(
		const Stroke& stroke,
		const util::rect<double>& /*roi*/,
		unsigned long beginStroke,
		unsigned long endStroke) {

	if (beginStroke == 0 && endStroke == 0) {

		beginStroke = stroke.begin();
		endStroke   = stroke.end();
	}

	// make sure there are enough points in this stroke to draw it
	if (stroke.end() - stroke.begin() <= 1) {

		LOG_ALL(skiastrokepainterlog) << "this stroke has less than two points -- skip drawing" << std::endl;
		return;
	}

	_canvas.save();
	_canvas.translate(stroke.getShift().x, stroke.getShift().y);
	_canvas.scale(stroke.getScale().x, stroke.getScale().y);

	double penWidth = stroke.getStyle().width();
	unsigned char penColorRed   = stroke.getStyle().getRed();
	unsigned char penColorGreen = stroke.getStyle().getGreen();
	unsigned char penColorBlue  = stroke.getStyle().getBlue();

	SkPaint paint;
	paint.setStrokeCap(SkPaint::kRound_Cap);
	paint.setColor(SkColorSetRGB(penColorRed, penColorGreen, penColorBlue));
	paint.setAntiAlias(true);

	// for each line in the stroke
	for (unsigned long i = beginStroke; i < endStroke - 1; i++) {

		//double alpha = alphaPressureCurve(stroke[i].pressure);
		double width = widthPressureCurve(_strokePoints[i].pressure);

		paint.setStrokeWidth(width*penWidth);

		_canvas.drawLine(
				_strokePoints[i  ].position.x, _strokePoints[i  ].position.y,
				_strokePoints[i+1].position.x, _strokePoints[i+1].position.y,
				paint);
	}

	_canvas.restore();

	return;
}

double
SkiaStrokePainter::widthPressureCurve(double pressure) {

	const double minPressure = 0.5;
	const double maxPressure = 1;

	pressure /= 2048.0;

	return minPressure + pressure*(maxPressure - minPressure);
}

double
SkiaStrokePainter::alphaPressureCurve(double pressure) {

	const double minPressure = 1;
	const double maxPressure = 1;

	pressure /= 2048.0;

	return minPressure + pressure*(maxPressure - minPressure);
}
