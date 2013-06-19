#include <util/Logger.h>
#include "Strokes.h"

logger::LogChannel strokeslog("strokeslog", "[Strokes] ");

util::rect<double>
Strokes::erase(const util::point<double>& begin, const util::point<double>& end) {

	//LOG_ALL(strokeslog) << "erasing strokes that intersect line from " << begin << " to " << end << std::endl;

	util::rect<double> eraseBoundingBox(
			begin.x,
			begin.y,
			begin.x,
			begin.y);
	eraseBoundingBox.fit(end);

	util::rect<double> changedArea(0, 0, 0, 0);

	unsigned int n = numStrokes();

	for (unsigned int i = 0; i < n; i++)
		if (getStroke(i).boundingBox().intersects(eraseBoundingBox)) {

			//LOG_ALL(strokeslog) << "stroke " << i << " is close to the erase position" << std::endl;

			util::rect<double> changedStrokeArea = erase(&getStroke(i), begin, end);

			if (changedArea.isZero()) {

				changedArea = changedStrokeArea;

			} else {

				if (!changedStrokeArea.isZero()) {

					changedArea.minX = std::min(changedArea.minX, changedStrokeArea.minX);
					changedArea.minY = std::min(changedArea.minY, changedStrokeArea.minY);
					changedArea.maxX = std::max(changedArea.maxX, changedStrokeArea.maxX);
					changedArea.maxY = std::max(changedArea.maxY, changedStrokeArea.maxY);
				}
			}
		}

	LOG_ALL(strokeslog) << "changed area is " << changedArea << std::endl;

	return changedArea;
}

util::rect<double>
Strokes::erase(const util::point<double>& position, double radius) {

	//LOG_ALL(strokeslog) << "erasing at " << position << " with radius " << radius << std::endl;

	util::rect<double> eraseBoundingBox(
			position.x - radius,
			position.y - radius,
			position.x + radius,
			position.y + radius);

	util::rect<double> changedArea(0, 0, 0, 0);

	unsigned int n = numStrokes();

	for (unsigned int i = 0; i < n; i++)
		if (getStroke(i).boundingBox().intersects(eraseBoundingBox)) {

			//LOG_ALL(strokeslog) << "stroke " << i << " is close to the erase position" << std::endl;

			util::rect<double> changedStrokeArea = erase(&getStroke(i), position, radius*radius);

			if (changedArea.isZero()) {

				changedArea = changedStrokeArea;

			} else {

				if (!changedStrokeArea.isZero()) {

					changedArea.minX = std::min(changedArea.minX, changedStrokeArea.minX);
					changedArea.minY = std::min(changedArea.minY, changedStrokeArea.minY);
					changedArea.maxX = std::max(changedArea.maxX, changedStrokeArea.maxX);
					changedArea.maxY = std::max(changedArea.maxY, changedStrokeArea.maxY);
				}
			}
		}

	LOG_ALL(strokeslog) << "changed area is " << changedArea << std::endl;

	return changedArea;
}

util::rect<double>
Strokes::erase(Stroke* stroke, const util::point<double>& center, double radius2) {

	util::rect<double> changedArea(0, 0, 0, 0);

	if (stroke->begin() == stroke->end())
		return changedArea;

	unsigned long begin = stroke->begin();
	unsigned long end   = stroke->end() - 1;

	LOG_ALL(strokeslog) << "testing stroke lines " << begin << " until " << (end - 1) << std::endl;

	Style style = stroke->getStyle();
	bool wasErasing = false;

	// for each line in the stroke
	for (unsigned long i = begin; i < end; i++) {

		// this line should be erased
		if (intersectsErasorCircle(_strokePoints[i].position, _strokePoints[i+1].position, center, radius2)) {

			LOG_ALL(strokeslog) << "line " << i << " needs to be erased" << std::endl;

			// update the changed area
			if (changedArea.isZero()) {

				changedArea.minX = _strokePoints[i].position.x;
				changedArea.minY = _strokePoints[i].position.y;
				changedArea.maxX = _strokePoints[i].position.x;
				changedArea.maxY = _strokePoints[i].position.y;
			}

			changedArea.fit(_strokePoints[i].position);
			changedArea.fit(_strokePoints[i+1].position);

			if (changedArea.isZero())
				LOG_ERROR(strokeslog) << "the change area is empty for line " << _strokePoints[i].position << " -- " << _strokePoints[i+1].position << std::endl;

			// if this is the first line to delete, we have to split
			if (!wasErasing) {

				LOG_ALL(strokeslog) << "this is the first line to erase on this stroke" << std::endl;

				stroke->setEnd(i+1);
				stroke->finish(_strokePoints);
				wasErasing = true;
			}

		// this line should be kept
		} else if (wasErasing) {

			LOG_ALL(strokeslog) << "line " << i << " is the next line not to erase on this stroke" << std::endl;

			createNewStroke(i);
			stroke = &(currentStroke());
			stroke->setStyle(style);
			wasErasing = false;
		}
	}

	// if we didn't split at all, this is a no-op, otherwise we finish the last 
	// stroke
	if (!wasErasing) {

		stroke->setEnd(end+1);
		stroke->finish(_strokePoints);
	}

	// increase the size of the changedArea (if there is one) by the style width
	if (!changedArea.isZero()) {

		changedArea.minX -= style.width();
		changedArea.minY -= style.width();
		changedArea.maxX += style.width();
		changedArea.maxY += style.width();
	}

	LOG_ALL(strokeslog) << "done erasing this stroke, changed area is " << changedArea << std::endl;

	return changedArea;
}

util::rect<double>
Strokes::erase(Stroke* stroke, const util::point<double>& lineBegin, const util::point<double>& lineEnd) {

	util::rect<double> changedArea(0, 0, 0, 0);

	if (stroke->begin() == stroke->end())
		return changedArea;

	unsigned long begin = stroke->begin();
	unsigned long end   = stroke->end() - 1;

	LOG_ALL(strokeslog) << "testing stroke lines " << begin << " until " << (end - 1) << std::endl;

	// for each line in the stroke
	for (unsigned long i = begin; i < end; i++) {

		// this line should be erased
		if (intersectLines(
				_strokePoints[i].position,
				_strokePoints[i+1].position - _strokePoints[i].position,
				lineBegin,
				lineEnd - lineBegin)) {

			LOG_ALL(strokeslog) << "this stroke needs to be erased" << std::endl;

			changedArea = stroke->boundingBox();

			// make this an empty stroke
			stroke->setEnd(begin);
			stroke->finish(_strokePoints);

			break;
		}
	}

	return changedArea;
}

bool
Strokes::intersectsErasorCircle(
		const util::point<double> lineStart,
		const util::point<double> lineEnd,
		const util::point<double> center,
		double radius2) {

	// if either of the points are in the circle, the line intersects
	util::point<double> diff = center - lineStart;
	if (diff.x*diff.x + diff.y*diff.y < radius2)
		return true;
	diff = center - lineEnd;
	if (diff.x*diff.x + diff.y*diff.y < radius2)
		return true;

	// see if the closest point on the line is in the circle

	// the line
	util::point<double> lineVector = lineEnd - lineStart;
	double lenLineVector = sqrt(lineVector.x*lineVector.x + lineVector.y*lineVector.y);

	// unit vector in the line's direction
	util::point<double> lineDirection = lineVector/lenLineVector;

	// the direction to the erasor
	util::point<double> erasorVector = center - lineStart;
	double lenErasorVector2 = erasorVector.x*erasorVector.x + erasorVector.y*erasorVector.y;

	// the dotproduct gives the distance from _strokePoints[i] to the closest 
	// point on the line to the erasor
	double a = lineDirection.x*erasorVector.x + lineDirection.y*erasorVector.y;

	// if a is beyond the beginning of end of the line, the line does not 
	// intersect the circle (since the beginning and end are not in the circle)
	if (a <= 0 || a >= lenLineVector)
		return false;

	// get the distance of the closest point to the center
	double erasorDistance2 = lenErasorVector2 - a*a;

	if (erasorDistance2 < radius2)
		return true;

	return false;
}

bool
Strokes::intersectLines(
		const util::point<double>& p,
		const util::point<double>& r,
		const util::point<double>& q,
		const util::point<double>& s) {

	// Line 1 is p + t*r, line 2 is q + u*s. We want to find t and u, such that 
	// p + t*r = q + u*s.

	// cross product between r and s
	double rXs = r.x*s.y - r.y*s.x;

	// vector from p to q
	const util::point<double> pq = q - p;

	// cross product of pq with s and r
	double pqXs = pq.x*s.y - pq.y*s.x;
	double pqXr = pq.x*r.y - pq.y*r.x;

	double t = pqXs/rXs;
	double u = pqXr/rXs;

	// only if both t and u are between 0 and 1 the lines intersected
	return t >= 0 && t <= 1 && u >= 0 && u <= 1;
}
