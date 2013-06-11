#include <util/Logger.h>
#include "CairoCanvasPainter.h"

logger::LogChannel cairocanvaspainterlog("cairocanvaspainterlog", "[CairoCanvasPainter] ");

CairoCanvasPainter::CairoCanvasPainter() :
	_pixelsPerDeviceUnit(1.0, 1.0),
	_pixelOffset(0, 0),
	_drawnUntilStroke(0),
	_drawnUntilStrokePoint(0),
	_drawnUntilStrokeTmp(0),
	_drawnUntilStrokePointTmp(0) {}

void
CairoCanvasPainter::draw(cairo_t* context) {

	util::rect<double> roi(0, 0, 0, 0);
	draw(context, roi);
}

void
CairoCanvasPainter::draw(
		cairo_t* context,
		const util::rect<double>& roi) {

	bool incremental = (_drawnUntilStroke != 0 || _drawnUntilStrokePoint != 0);

	LOG_ALL(cairocanvaspainterlog) << "drawing " << (incremental ? "" : "non-") << "incrementally" << std::endl;

	cairo_save(context);

	util::rect<double> canvasRoi(0, 0, 0, 0);

	if (roi.area() != 0) {

		// clip outside our responsibility
		cairo_rectangle(context, roi.minX, roi.minY, roi.width(), roi.height());
		cairo_clip(context);

		// transform roi to canvas units
		canvasRoi = roi;
		canvasRoi -= _pixelOffset;
		canvasRoi /= _pixelsPerDeviceUnit;
	}

	// apply the given transformation

	// translate should be performed...
	cairo_translate(context, _pixelOffset.x, _pixelOffset.y);
	// ...after the scaling
	cairo_scale(context, _pixelsPerDeviceUnit.x, _pixelsPerDeviceUnit.y);

	// prepare the background, if we do not draw incrementally
	if (!incremental)
		clearSurface(context);

	// draw the (new) strokes in the current part
	unsigned int drawnUntilStrokePoint = (incremental ? _drawnUntilStrokePoint : 0);
	for (unsigned int i = (incremental ? _drawnUntilStroke : 0); i < _strokes->size(); i++) {

		LOG_ALL(cairocanvaspainterlog)
				<< "drawing stroke " << i << ", starting from point "
				<< drawnUntilStrokePoint << std::endl;

		const Stroke& stroke = (*_strokes)[i];

		// draw only of no roi given or stroke intersects roi
		if (canvasRoi.width() == 0 || stroke.boundingBox().intersects(canvasRoi))
			drawStroke(context, stroke, canvasRoi, drawnUntilStrokePoint);

		drawnUntilStrokePoint = 0;
	}

	cairo_restore(context);

	// temporarilly remember what we drew already
	_drawnUntilStrokeTmp = std::max(0, static_cast<int>(_strokes->size()) - 1);

	if (_strokes->size() > 0)
		_drawnUntilStrokePointTmp = std::max(0, static_cast<int>((*_strokes)[_drawnUntilStrokeTmp].size()) - 1);
	else
		_drawnUntilStrokePointTmp = 0;
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
	double penWidth = stroke.width();
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
	cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(context, 1.0, 1.0, 1.0, 1.0);
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
