#ifndef YANTA_SKIA_STROKE_LINE_PAINTER_H__
#define YANTA_SKIA_STROKE_LINE_PAINTER_H__

#include <util/rect.hpp>
#include "Quality.h"

// forward declarations
class SkCanvas;
class Stroke;
class StrokePoints;

class SkiaStrokeLinePainter {

public:

	SkiaStrokeLinePainter() : _quality(Best) {}

	void draw(
		SkCanvas& canvas,
		const StrokePoints& strokePoints,
		const Stroke& stroke,
		const util::rect<double>& roi,
		unsigned long beginStroke = 0,
		unsigned long endStroke   = 0);

	/**
	 * Set the quality of this rasterizer.
	 */
	void setQuality(Quality quality) { _quality = quality; }

	/**
	 * Get the quality of this rasterizer.
	 */
	inline Quality getQuality() { return _quality; }

private:

	double widthPressureCurve(double pressure);

	double alphaPressureCurve(double pressure);

	Quality _quality;
};

#endif // YANTA_SKIA_STROKE_PAINTER_H__

