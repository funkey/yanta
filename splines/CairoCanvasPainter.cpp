#include "CairoCanvasPainter.h"


void
CairoCanvasPainter::draw(cairo_t* context) {

	// TODO: get Strokes bounding box
	util::rect<double> roi(0, 0, 1, 1);

	draw(context, roi);
}

void
CairoCanvasPainter::draw(
		cairo_t* context,
		const util::rect<double>& roi,
		unsigned int drawnUntilStroke,
		unsigned int drawnUntilStrokePoint) {

	// clip outside our responsibility
	cairo_rectangle(context, roi.minX, roi.minY, roi.width(), roi.height());
	cairo_clip(context);

	// prepare the background, if we do not draw incrementally
	if (drawnUntilStroke == 0 && drawnUntilStrokePoint == 0)
		clearSurface(context);

	// draw the (new) strokes in the current part
	for (unsigned int i = drawnUntilStroke; i < _strokes->size(); i++) {

		drawStroke(context, (*_strokes)[i], roi, drawnUntilStrokePoint);
		drawnUntilStrokePoint = 0;
	}
}

void
CairoCanvasPainter::drawStroke(
		cairo_t* context,
		const Stroke& stroke,
		const util::rect<double>& /*roi*/,
		unsigned int drawnUntilStrokePoint) {

	if (stroke.size() == 0)
		return;

	// TODO: read this from stroke data structure
	double penWidth = 2.0;
	double penColorRed   = 0;
	double penColorGreen = 0;
	double penColorBlue  = 0;

	cairo_set_line_cap(context, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(context, CAIRO_LINE_JOIN_ROUND);

	for (unsigned int i = drawnUntilStrokePoint + 1; i < stroke.size(); i++) {

		double alpha = alphaPressureCurve(stroke[i].pressure);
		double width = widthPressureCurve(stroke[i].pressure);

		cairo_set_source_rgba(context, penColorRed, penColorGreen, penColorBlue, alpha);
		cairo_set_line_width(context, width*penWidth);

		cairo_move_to(context, stroke[i-1].position.x, stroke[i-1].position.y);
		cairo_line_to(context, stroke[i  ].position.x, stroke[i  ].position.y);
		cairo_stroke(context);
	}
}

void
CairoCanvasPainter::clearSurface(cairo_t* context) {

	// clear surface
	cairo_set_operator(context, CAIRO_OPERATOR_CLEAR);
	cairo_paint(context);
	cairo_set_operator(context, CAIRO_OPERATOR_OVER);
}

double
CairoCanvasPainter::widthPressureCurve(double pressure) {

	const double minPressure = 0.0;
	const double maxPressure = 1.5;

	pressure /= 2048.0;

	return minPressure + pressure*(maxPressure - minPressure);
}

double
CairoCanvasPainter::alphaPressureCurve(double pressure) {

	const double minPressure = 1;
	const double maxPressure = 1;

	pressure /= 2048.0;

	return minPressure + pressure*(maxPressure - minPressure);
}
