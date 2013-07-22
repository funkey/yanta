#include "Stroke.h"

void
Stroke::computeBoundingBox(const StrokePoints& points) {

	resetBoundingBox();

	for (unsigned long i = _begin; i < _end; i++) {

		const StrokePoint& point = points[i];

		fitBoundingBox(util::rect<DocumentPrecision>(
				point.position.x - _style.width(),
				point.position.y - _style.width(),
				point.position.x + _style.width(),
				point.position.y + _style.width()));
	}
}
