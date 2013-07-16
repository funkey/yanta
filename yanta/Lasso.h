#ifndef YANTA_LASSO_H__
#define YANTA_LASSO_H__

#include "OverlayObject.h"
#include "SkPath.h"

/**
 * A lasso for the selection of canvas entities.
 */
class Lasso : public OverlayObject {

public:

	Lasso() {

		_path.setFillType(SkPath::kWinding_FillType);
	}

	void addPoint(const util::point<CanvasPrecision>& point) {

		if (_path.countPoints() == 0) {

			_path.moveTo(SkPoint::Make(point.x, point.y));
			_roi.minX = point.x;
			_roi.minY = point.y;
			_roi.maxX = point.x;
			_roi.maxY = point.y;

		} else {

			_path.lineTo(SkPoint::Make(point.x, point.y));
			_roi.fit(point);
		}
	}

	bool contains(const util::point<CanvasPrecision>& point) {

		return _path.contains(point.x, point.y);
	}

	const util::rect<CanvasPrecision>& getRoi() const { return _roi; }

	SkPath& getPath() { return _path; }
	const SkPath& getPath() const { return _path; }

	void visit(OverlayObjectVisitor& visitor) { visitor.processLasso(*this); }

private:

	SkPath _path;

	util::rect<CanvasPrecision> _roi;
};

#endif // YANTA_LASSO_H__

