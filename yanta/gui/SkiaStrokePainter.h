#ifndef YANTA_SKIA_STROKE_PAINTER_H__
#define YANTA_SKIA_STROKE_PAINTER_H__

#include <util/rect.hpp>

// forward declarations
class SkCanvas;
class Stroke;
class StrokePoints;

class SkiaStrokePainter {

public:

	SkiaStrokePainter(SkCanvas& canvas, const StrokePoints& strokePoints);

	void draw(
		const Stroke& stroke,
		const util::rect<double>& roi,
		unsigned long beginStroke = 0,
		unsigned long endStroke   = 0);

private:

	double widthPressureCurve(double pressure);

	double alphaPressureCurve(double pressure);

	SkCanvas&           _canvas;
	const StrokePoints& _strokePoints;
};

#endif // YANTA_SKIA_STROKE_PAINTER_H__

