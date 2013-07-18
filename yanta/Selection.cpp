#include <boost/bind.hpp>
#include <util/Logger.h>
#include "Canvas.h"
#include "Selection.h"
#include "Path.h"

logger::LogChannel selectionlog("selectionlog", "[Selection] ");

Selection
Selection::CreateFromPath(const Path& path, Canvas& canvas) {

	Selection selection(canvas.getStrokePoints());

	for (unsigned int p = 0; p < canvas.numPages(); p++) {

		Page& page = canvas.getPage(p);

		// get all the strokes that are fully contained in the path
		std::vector<Stroke> selectedStrokes = page.removeStrokes(boost::bind(&Path::contains, &path, boost::ref(page), _1, boost::ref(canvas.getStrokePoints())));

		for (std::vector<Stroke>::iterator i = selectedStrokes.begin(); i != selectedStrokes.end(); i++)
			selection.addStroke(page, *i);
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

		LOG_DEBUG(selectionlog) << "anchoring stroke " << i << " at " << stroke.getShift() << " in selection at " << getShift() << std::endl;

		// correct for the transformation applied to the selection
		stroke.scale(getScale());
		stroke.shift(getShift());

		// get the page that is closest to the center of the stroke
		unsigned int p = canvas.getPageIndex(stroke.getBoundingBox().center());
		Page& page = canvas.getPage(p);

		LOG_DEBUG(selectionlog) << "page " << p << " is closest" << std::endl;

		// correct for the position of the page
		stroke.shift(-page.getPosition());

		LOG_DEBUG(selectionlog) << "relative to page, stroke is now at " << stroke.getShift() << std::endl;

		// add it
		canvas.getPage(p).addStroke(stroke);
	}
}
