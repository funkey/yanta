#include "Canvas.h"
#include "Page.h"
#include <util/Logger.h>

logger::LogChannel pagelog("pagelog", "[Page] ");

Page::Page(
		Canvas* canvas,
		const util::point<CanvasPrecision>& position,
		const util::point<PagePrecision>&   size) :
	_position(position),
	_size(size),
	_pageBoundingBox(position.x, position.y, position.x + size.x, position.y + size.y),
	_strokePoints(canvas->getStrokePoints()) {}

Page&
Page::operator=(const Page& other) {

	_position        = other._position;
	_size            = other._size;
	_pageBoundingBox = other._pageBoundingBox;
	_strokes         = other._strokes;

	// we don't copy the stroke points, since they might belong to another 
	// canvas
	return *this;
}

void
Page::createNewStroke(
		const util::point<CanvasPrecision>& start,
		double                              pressure,
		unsigned long                       timestamp) {

	createNewStroke(_strokePoints.size());
	addStrokePoint(start, pressure, timestamp);
}

void
Page::createNewStroke(unsigned long begin) {

	LOG_ALL(pagelog) << "creating a new stroke starting at " << begin << std::endl;

	// if this happens in the middle of a draw, finish the unfinished
	if (numStrokes() > 0 && !currentStroke().finished())
		currentStroke().finish(_strokePoints);

	_strokes.push_back(Stroke(begin));
}

util::rect<CanvasPrecision>
Page::erase(const util::point<CanvasPrecision>& begin, const util::point<CanvasPrecision>& end) {

	//LOG_ALL(pagelog) << "erasing strokes that intersect line from " << begin << " to " << end << std::endl;

	// get the erase line in page coordinates
	const util::point<PagePrecision> pageBegin = toPageCoordinates(begin);
	const util::point<PagePrecision> pageEnd   = toPageCoordinates(end);

	util::rect<PagePrecision> eraseBoundingBox(
			pageBegin.x,
			pageBegin.y,
			pageBegin.x,
			pageBegin.y);
	eraseBoundingBox.fit(pageEnd);

	util::rect<PagePrecision> changedArea(0, 0, 0, 0);

	unsigned int n = numStrokes();

	for (unsigned int i = 0; i < n; i++)
		if (getStroke(i).getBoundingBox().intersects(eraseBoundingBox)) {

			//LOG_ALL(pagelog) << "stroke " << i << " is close to the erase position" << std::endl;

			util::rect<PagePrecision> changedStrokeArea = erase(getStroke(i), pageBegin, pageEnd);

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

	LOG_ALL(pagelog) << "changed area is " << changedArea << std::endl;

	// transform the changed area to canvas coordinates
	return toCanvasCoordinates(changedArea);
}

util::rect<CanvasPrecision>
Page::erase(const util::point<PagePrecision>& position, PagePrecision radius) {

	//LOG_ALL(pagelog) << "erasing at " << position << " with radius " << radius << std::endl;

	// get the erase position in page coordinates
	const util::point<PagePrecision> pagePosition = toPageCoordinates(position);

	util::rect<PagePrecision> eraseBoundingBox(
			pagePosition.x - radius,
			pagePosition.y - radius,
			pagePosition.x + radius,
			pagePosition.y + radius);

	util::rect<PagePrecision> changedArea(0, 0, 0, 0);

	unsigned int n = numStrokes();

	for (unsigned int i = 0; i < n; i++)
		if (getStroke(i).getBoundingBox().intersects(eraseBoundingBox)) {

			//LOG_ALL(pagelog) << "stroke " << i << " is close to the erase pagePosition" << std::endl;

			util::rect<PagePrecision> changedStrokeArea = erase(&getStroke(i), pagePosition, radius*radius);

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

	LOG_ALL(pagelog) << "changed area is " << changedArea << std::endl;

	// transform the changed area to canvas coordinates
	return toCanvasCoordinates(changedArea);
}

util::rect<PagePrecision>
Page::erase(Stroke* stroke, const util::point<PagePrecision>& center, PagePrecision radius2) {

	util::rect<PagePrecision> changedArea(0, 0, 0, 0);

	if (stroke->begin() == stroke->end())
		return changedArea;

	unsigned long begin = stroke->begin();
	unsigned long end   = stroke->end() - 1;

	LOG_ALL(pagelog) << "testing stroke lines " << begin << " until " << (end - 1) << std::endl;

	Style style = stroke->getStyle();
	bool wasErasing = false;

	// for each line in the stroke
	for (unsigned long i = begin; i < end; i++) {

		// this line should be erased
		if (intersectsErasorCircle(_strokePoints[i].position, _strokePoints[i+1].position, center, radius2)) {

			LOG_ALL(pagelog) << "line " << i << " needs to be erased" << std::endl;

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
				LOG_ERROR(pagelog) << "the change area is empty for line " << _strokePoints[i].position << " -- " << _strokePoints[i+1].position << std::endl;

			// if this is the first line to delete, we have to split
			if (!wasErasing) {

				LOG_ALL(pagelog) << "this is the first line to erase on this stroke" << std::endl;

				stroke->setEnd(i+1);
				stroke->finish(_strokePoints);
				wasErasing = true;
			}

		// this line should be kept
		} else if (wasErasing) {

			LOG_ALL(pagelog) << "line " << i << " is the next line not to erase on this stroke" << std::endl;

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

	LOG_ALL(pagelog) << "done erasing this stroke, changed area is " << changedArea << std::endl;

	return changedArea;
}

util::rect<PagePrecision>
Page::erase(Stroke& stroke, const util::point<PagePrecision>& lineBegin, const util::point<PagePrecision>& lineEnd) {

	util::rect<PagePrecision> changedArea(0, 0, 0, 0);

	if (stroke.begin() == stroke.end())
		return changedArea;

	unsigned long begin = stroke.begin();
	unsigned long end   = stroke.end() - 1;

	LOG_ALL(pagelog) << "testing stroke lines " << begin << " until " << (end - 1) << std::endl;

	util::point<PagePrecision> startPoint = _strokePoints[begin].position*stroke.getScale() + stroke.getShift();

	// for each line in the stroke
	for (unsigned long i = begin; i < end; i++) {

		util::point<PagePrecision> endPoint = _strokePoints[i+1].position*stroke.getScale() + stroke.getShift();

		// this line should be erased
		if (intersectLines(
				startPoint,
				endPoint - startPoint,
				lineBegin,
				lineEnd - lineBegin)) {

			LOG_ALL(pagelog) << "this stroke needs to be erased" << std::endl;

			changedArea = stroke.getBoundingBox();

			// make this an empty stroke
			stroke.setEnd(begin);
			stroke.finish(_strokePoints);

			break;
		}

		startPoint = endPoint;
	}

	return changedArea;
}

bool
Page::intersectsErasorCircle(
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
Page::intersectLines(
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
