#include <boost/bind.hpp>
#include "Canvas.h"
#include "Selection.h"
#include "Path.h"

Selection
Selection::CreateFromPath(const Path& path, Canvas& canvas) {

	Selection selection(canvas.getStrokePoints());

	for (unsigned int p = 0; p < canvas.numPages(); p++) {

		Page& page = canvas.getPage(p);

		// get all the strokes that are fully contained in the path
		std::vector<Stroke> selectedStrokes = page.removeStrokes(boost::bind(&Path::contains, &path, _1, boost::ref(canvas.getStrokePoints())));

		for (std::vector<Stroke>::iterator i = selectedStrokes.begin(); i != selectedStrokes.end(); i++) {

			i->shift(page.getPosition());
			selection.addStroke(page, *i);
		}
	}

	return selection;
}

Selection::Selection(const StrokePoints& strokePoints) :
	_strokePoints(strokePoints) {}

void
Selection::addStroke(const Page& page, const Stroke& stroke) {

	// to detach the stroke from the page, we have to shift it by the page 
	// position
	Stroke selectionCopy = stroke;
	selectionCopy.shift(page.getPosition());

	fitBoundingBox(selectionCopy.getBoundingBox());

	_strokes.push_back(selectionCopy);
}

void
Selection::anchor(Canvas& canvas) {

	for (unsigned int i = 0; i < numStrokes(); i++) {

		Stroke& stroke = getStroke(i);

		// get the page that is closest to the center of the stroke
		unsigned int p = canvas.getPageIndex(stroke.getBoundingBox().center());
		Page& page = canvas.getPage(p);

		// correct for the transformation applied to the selection
		stroke.shift(getShift());
		stroke.scale(getScale());

		// correct for the position of the page
		stroke.shift(-page.getPosition());

		// add it
		canvas.getPage(p).addStroke(stroke);
	}
}
