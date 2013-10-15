#ifndef YANTA_LASSO_H__
#define YANTA_LASSO_H__

#include <document/Path.h>
#include "Tool.h"

/**
 * A lasso for the selection of document entities.
 */
class Lasso : public Tool {

public:

	Lasso() {

		_path.setFillType(SkPath::kEvenOdd_FillType);
	}

	void addPoint(const util::point<DocumentPrecision>& point) {

		fitBoundingBox(point);

		if (_path.countPoints() == 0)
			_path.moveTo(SkPoint::Make(point.x, point.y));
		else
			_path.lineTo(SkPoint::Make(point.x, point.y));
	}

	Path& getPath() { return _path; }
	const Path& getPath() const { return _path; }

	/**
	 * Draw this tool on the given canvas.
	 */
	void draw(SkCanvas& canvas);

private:

	Path _path;
};

#endif // YANTA_LASSO_H__

