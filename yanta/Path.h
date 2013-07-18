#ifndef YANTA_PATH_H__
#define YANTA_PATH_H__

#include <SkPath.h>
#include "Precision.h"
#include "Page.h"
#include "Stroke.h"
#include "StrokePoints.h"

/**
 * We are using the skia path as our path structure, since it provides already 
 * what we need (easy interface for drawing and containment test).
 *
 * If at any point we want to use another drawing library, this is where we 
 * should define our own path class.
 */
class Path : public SkPath {

public:

	Path() {

		SkPath::setFillType(SkPath::kWinding_FillType);
	}

	/**
	 * Test, whether a point is contained in this path.
	 */
	bool contains(const util::point<CanvasPrecision>& point) const {

		return SkPath::contains(point.x, point.y);
	}

	/**
	 * Test, whether a stroke is fully contained in this path.
	 */
	bool contains(const Page& page, const Stroke& stroke, const StrokePoints& points) const {

		if (stroke.size() == 0)
			return false;

		for (unsigned int i = stroke.begin(); i < stroke.end(); i++) {

			util::point<CanvasPrecision> point = points[i].position*stroke.getScale() + stroke.getShift() + page.getPosition();

			if (!contains(point))
				return false;
		}

		return true;
	}
};

#endif // YANTA_PATH_H__

