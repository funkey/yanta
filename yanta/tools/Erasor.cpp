#include <util/Logger.h>
#include "Erasor.h"

logger::LogChannel erasorlog("erasorlog", "[Erasor] ");

Erasor::Erasor(Document& document, ErasorMode mode) :
	_document(document),
	_strokePoints(document.getStrokePoints()),
	_mode(mode) {}

util::rect<DocumentPrecision>
Erasor::erase(
		const util::point<DocumentPrecision>& start,
		const util::point<DocumentPrecision>& end) {

	LOG_ALL(erasorlog) << "starting erasing from " << start << " - " << end << std::endl;

	// set roi for document tree traversal
	util::rect<DocumentPrecision> roi(start.x, start.y, start.x, start.y);
	roi.fit(end);
	roi.minX -= _radius;
	roi.minY -= _radius;
	roi.maxX += _radius;
	roi.maxY += _radius;
	setRoi(roi);

	// initialize erasor line in document units
	_start = start;
	_end   = end;

	// clear changed area
	_changed = util::rect<DocumentPrecision>(0, 0, 0, 0);

	// visit the document
	_document.accept(*this);

	return _changed;
}

void
Erasor::visit(Stroke& stroke) {

	LOG_ALL(erasorlog) << "visiting stroke with erasor line " << _start << " - " << _end << std::endl;

	util::point<PagePrecision> start = getTransformation().getInverse().applyTo(_start);
	util::point<PagePrecision> end   = getTransformation().getInverse().applyTo(_end);

	LOG_ALL(erasorlog) << "in stroke stroke coordinates this is " << start << " - " << end << std::endl;

	util::rect<PagePrecision> changed = erase(stroke, start, end);

	if (changed.isZero())
		return;

	if (_changed.isZero())
		_changed = getTransformation().applyTo(changed);
	else
		_changed.fit(getTransformation().applyTo(changed));
}

#if 0
util::rect<PagePrecision>
Erasor::erase(Stroke* stroke, const util::point<PagePrecision>& center, PagePrecision radius2) {

	util::rect<PagePrecision> changedArea(0, 0, 0, 0);

	if (stroke->begin() == stroke->end())
		return changedArea;

	unsigned long begin = stroke->begin();
	unsigned long end   = stroke->end() - 1;

	LOG_ALL(erasorlog) << "testing stroke lines " << begin << " until " << (end - 1) << std::endl;

	Style style = stroke->getStyle();
	bool wasErasing = false;

	// for each line in the stroke
	for (unsigned long i = begin; i < end; i++) {

		// this line should be erased
		if (intersectsErasorCircle(_strokePoints[i].position, _strokePoints[i+1].position, center, radius2)) {

			LOG_ALL(erasorlog) << "line " << i << " needs to be erased" << std::endl;

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
				LOG_ERROR(erasorlog) << "the change area is empty for line " << _strokePoints[i].position << " -- " << _strokePoints[i+1].position << std::endl;

			// if this is the first line to delete, we have to split
			if (!wasErasing) {

				LOG_ALL(erasorlog) << "this is the first line to erase on this stroke" << std::endl;

				stroke->setEnd(i+1, _strokePoints);
				stroke->finish();
				wasErasing = true;
			}

		// this line should be kept
		} else if (wasErasing) {

			LOG_ALL(erasorlog) << "line " << i << " is the next line not to erase on this stroke" << std::endl;

			// TODO:
			//   find a way to create a new stroke from here
			//createNewStroke(i);
			//stroke = &(currentStroke());
			stroke->setStyle(style);
			wasErasing = false;
		}
	}

	// if we didn't split at all, this is a no-op, otherwise we finish the last 
	// stroke
	if (!wasErasing) {

		stroke->setEnd(end+1, _strokePoints);
		stroke->finish();
	}

	// increase the size of the changedArea (if there is one) by the style width
	if (!changedArea.isZero()) {

		changedArea.minX -= style.width();
		changedArea.minY -= style.width();
		changedArea.maxX += style.width();
		changedArea.maxY += style.width();
	}

	LOG_ALL(erasorlog) << "done erasing this stroke, changed area is " << changedArea << std::endl;

	return changedArea;
}
#endif

util::rect<PagePrecision>
Erasor::erase(Stroke& stroke, const util::point<PagePrecision>& lineBegin, const util::point<PagePrecision>& lineEnd) {

	util::rect<PagePrecision> changedArea(0, 0, 0, 0);

	if (stroke.begin() == stroke.end())
		return changedArea;

	unsigned long begin = stroke.begin();
	unsigned long end   = stroke.end() - 1;

	LOG_ALL(erasorlog) << "testing stroke lines " << begin << " until " << (end - 1) << std::endl;

	util::point<PagePrecision> startPoint = _strokePoints[begin].position;

	// for each line in the stroke
	for (unsigned long i = begin; i < end; i++) {

		util::point<PagePrecision> endPoint = _strokePoints[i+1].position;

		// this line should be erased
		if (intersectLines(
				startPoint,
				endPoint - startPoint,
				lineBegin,
				lineEnd - lineBegin)) {

			LOG_ALL(erasorlog) << "this stroke needs to be erased" << std::endl;

			changedArea = (stroke.getBoundingBox() - stroke.getShift())/stroke.getScale();

			// make this an empty stroke
			stroke.setEnd(begin, _strokePoints);
			stroke.finish();

			break;
		}

		startPoint = endPoint;
	}

	return changedArea;
}

bool
Erasor::intersectsErasorCircle(
		const util::point<PagePrecision> lineStart,
		const util::point<PagePrecision> lineEnd,
		const util::point<PagePrecision> center,
		PagePrecision radius2) {

	// if either of the points are in the circle, the line intersects
	util::point<PagePrecision> diff = center - lineStart;
	if (diff.x*diff.x + diff.y*diff.y < radius2)
		return true;
	diff = center - lineEnd;
	if (diff.x*diff.x + diff.y*diff.y < radius2)
		return true;

	// see if the closest point on the line is in the circle

	// the line
	util::point<PagePrecision> lineVector = lineEnd - lineStart;
	PagePrecision lenLineVector = sqrt(lineVector.x*lineVector.x + lineVector.y*lineVector.y);

	// unit vector in the line's direction
	util::point<PagePrecision> lineDirection = lineVector/lenLineVector;

	// the direction to the erasor
	util::point<PagePrecision> erasorVector = center - lineStart;
	PagePrecision lenErasorVector2 = erasorVector.x*erasorVector.x + erasorVector.y*erasorVector.y;

	// the dotproduct gives the distance from _strokePoints[i] to the closest 
	// point on the line to the erasor
	PagePrecision a = lineDirection.x*erasorVector.x + lineDirection.y*erasorVector.y;

	// if a is beyond the beginning of end of the line, the line does not 
	// intersect the circle (since the beginning and end are not in the circle)
	if (a <= 0 || a >= lenLineVector)
		return false;

	// get the distance of the closest point to the center
	PagePrecision erasorDistance2 = lenErasorVector2 - a*a;

	if (erasorDistance2 < radius2)
		return true;

	return false;
}

bool
Erasor::intersectLines(
		const util::point<PagePrecision>& p,
		const util::point<PagePrecision>& r,
		const util::point<PagePrecision>& q,
		const util::point<PagePrecision>& s) {

	// Line 1 is p + t*r, line 2 is q + u*s. We want to find t and u, such that 
	// p + t*r = q + u*s.

	// cross product between r and s
	PagePrecision rXs = r.x*s.y - r.y*s.x;

	// vector from p to q
	const util::point<PagePrecision> pq = q - p;

	// cross product of pq with s and r
	PagePrecision pqXs = pq.x*s.y - pq.y*s.x;
	PagePrecision pqXr = pq.x*r.y - pq.y*r.x;

	PagePrecision t = pqXs/rXs;
	PagePrecision u = pqXr/rXs;

	// only if both t and u are between 0 and 1 the lines intersected
	return t >= 0 && t <= 1 && u >= 0 && u <= 1;
}
