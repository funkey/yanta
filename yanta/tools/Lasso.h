#ifndef YANTA_LASSO_H__
#define YANTA_LASSO_H__

#include <document/Path.h>
#include <gui/OverlayObject.h>

/**
 * A lasso for the selection of document entities.
 */
class Lasso : public OverlayObject {

public:

	void addPoint(const util::point<DocumentPrecision>& point) {

		fitBoundingBox(point);

		if (_path.countPoints() == 0)
			_path.moveTo(SkPoint::Make(point.x, point.y));
		else
			_path.lineTo(SkPoint::Make(point.x, point.y));
	}

	Path& getPath() { return _path; }
	const Path& getPath() const { return _path; }

	void visit(OverlayObjectVisitor& visitor) { visitor.processLasso(*this); }

private:

	Path _path;
};

#endif // YANTA_LASSO_H__

