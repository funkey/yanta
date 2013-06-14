#include "Stroke.h"

void
Stroke::computeBoundingBox(const StrokePoints& points) {

	_boundingBox = util::rect<double>(0, 0, 0, 0);

	for (unsigned long i = _begin; i < _end; i++) {

		const StrokePoint& point = points[i];

		if (_boundingBox.width() == 0) {

			_boundingBox.minX = point.position.x - _pen.width();
			_boundingBox.minY = point.position.y - _pen.width();
			_boundingBox.maxX = point.position.x + _pen.width();
			_boundingBox.maxY = point.position.y + _pen.width();

		} else {

			_boundingBox.minX = std::min(_boundingBox.minX, point.position.x - _pen.width());
			_boundingBox.minY = std::min(_boundingBox.minY, point.position.y - _pen.width());
			_boundingBox.maxX = std::max(_boundingBox.maxX, point.position.x + _pen.width());
			_boundingBox.maxY = std::max(_boundingBox.maxY, point.position.y + _pen.width());
		}
	}
}
