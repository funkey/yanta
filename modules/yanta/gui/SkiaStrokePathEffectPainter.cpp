#include <SkCanvas.h>
#include <SkMaskFilter.h>
#include <SkBlurMaskFilter.h>
#include <SkPathEffect.h>
#include <Sk1DPathEffect.h>
#include <SkCornerPathEffect.h>

#include <document/Stroke.h>
#include <document/StrokePoints.h>
#include "SkiaStrokePathEffectPainter.h"
#include "util/Logger.h"

SkiaStrokePathEffectPainter::SkiaStrokePathEffectPainter(SkCanvas& canvas, const StrokePoints& strokePoints) :
	_canvas(canvas),
	_strokePoints(strokePoints) {}

SkPathEffect*
SkiaStrokePathEffectPainter::makePathEffect(
		const Stroke& stroke,
		unsigned int beginStroke,
		unsigned int endStroke) {

	double penWidth = 0.5*stroke.getStyle().width();

	util::point<PagePrecision> previousPosition = _strokePoints[beginStroke].position;
	double l = 0;

	SkPath path;
	path.moveTo(0, 0);

	static const int increment = 1;

	// forward direction
	long i = beginStroke;
	for (; i < endStroke; i += increment) {

		util::point<PagePrecision> position = _strokePoints[i].position;
		util::point<PagePrecision> diff = position - previousPosition;

		// length of the stroke until point i
		l += sqrt(diff.x*diff.x + diff.y*diff.y);

		path.lineTo(l, widthPressureCurve(_strokePoints[i].pressure)*penWidth);

		previousPosition = position;
	}
	i -= increment;

	double length = l;

	//backward direction
	for (; i >= beginStroke; i -= increment) {

		util::point<PagePrecision> position = _strokePoints[i].position;
		util::point<PagePrecision> diff = position - previousPosition;

		//length of the stroke until point i
		l -= sqrt(diff.x*diff.x + diff.y*diff.y);

		path.lineTo(l, -widthPressureCurve(_strokePoints[i].pressure)*penWidth);

		previousPosition = position;
	}
	path.lineTo(0, 0);
	path.close();

	//SkPath backward;
	//SkMatrix mirror;
	//mirror.setIdentity();
	//mirror.setScaleY(-1);
	//path.transform(mirror, &backward);
	//path.reverseAddPath(backward);

	SkPathEffect* pressureEffect = SkPath1DPathEffect::Create(path, length, 0, SkPath1DPathEffect::kMorph_Style);
#if 1
	return pressureEffect;
#else
	SkPathEffect* smoothEffect = new SkCornerPathEffect(SkIntToScalar(200));
	SkPathEffect* pathEffect   = new SkComposePathEffect(pressureEffect, smoothEffect);

	smoothEffect->unref();
	pressureEffect->unref();

	return pathEffect;
#endif
}

void
SkiaStrokePathEffectPainter::draw(
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

	unsigned char penColorRed   = stroke.getStyle().getRed();
	unsigned char penColorGreen = stroke.getStyle().getGreen();
	unsigned char penColorBlue  = stroke.getStyle().getBlue();

	SkPaint paint;
	paint.setStrokeCap(SkPaint::kRound_Cap);
	paint.setColor(SkColorSetRGB(penColorRed, penColorGreen, penColorBlue));
	paint.setAntiAlias(true);

	//SkMaskFilter* maskFilter = SkBlurMaskFilter::Create(0.05, SkBlurMaskFilter::kNormal_BlurStyle);
	//paint.setMaskFilter(maskFilter)->unref();

	SkPathEffect* pathEffect = makePathEffect(stroke, beginStroke, endStroke);
	paint.setPathEffect(pathEffect)->unref();

	SkPath path;
	path.moveTo(_strokePoints[beginStroke].position.x, _strokePoints[beginStroke].position.y);

	// for each line in the stroke
	for (unsigned long i = beginStroke + 1; i < endStroke; i++)
		path.lineTo(_strokePoints[i].position.x, _strokePoints[i].position.y);

	_canvas.drawPath(path, paint);

	return;
}

double
SkiaStrokePathEffectPainter::widthPressureCurve(double pressure) {

	const double minPressure = 0.5;
	const double maxPressure = 1;

	pressure /= 2048.0;

	return minPressure + pressure*(maxPressure - minPressure);
}

double
SkiaStrokePathEffectPainter::alphaPressureCurve(double pressure) {

	const double minPressure = 0;
	const double maxPressure = 1;

	pressure /= 2048.0;

	return minPressure + pressure*(maxPressure - minPressure);
}
