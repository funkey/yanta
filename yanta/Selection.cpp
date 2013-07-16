#include <boost/bind.hpp>
#include "Canvas.h"
#include "Selection.h"
#include "Path.h"

Selection::Selection() {}

Selection
Selection::CreateFromPath(const Path& path, Canvas& canvas) {

	Selection selection;

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

void
Selection::addStroke(const Page& page, const Stroke& stroke) {

	// to detach the stroke from the page, we have to shift it by the page 
	// position
	Stroke selectionCopy = stroke;
	selectionCopy.shift(page.getPosition());

	fitBoundingBox(selectionCopy.getBoundingBox());

	_strokes.push_back(selectionCopy);
}
