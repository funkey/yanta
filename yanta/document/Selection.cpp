#include <boost/bind.hpp>

#include <util/Logger.h>

#include "Document.h"
#include "Selection.h"
#include "Path.h"

logger::LogChannel selectionlog("selectionlog", "[Selection] ");

Selection
Selection::CreateFromPath(const Path& path, Document& document) {

	Selection selection(document.getStrokePoints());

	for (unsigned int p = 0; p < document.numPages(); p++) {

		Page& page = document.getPage(p);

		// get all the strokes that are fully contained in the path
		std::vector<Stroke> selectedStrokes = page.removeStrokes(boost::bind(&Path::contains, &path, boost::ref(page), _1, boost::ref(document.getStrokePoints())));

		for (std::vector<Stroke>::iterator i = selectedStrokes.begin(); i != selectedStrokes.end(); i++)
			selection.addStroke(page, *i);
	}

	return selection;
}

Selection::Selection(const StrokePoints& strokePoints) :
	_strokePoints(strokePoints) {}

Selection&
Selection::operator=(const Selection& other) {

	// copy the elements of the container
	DocumentElementContainer<SelectionElementTypes>::operator=(other);

	// we don't copy the stroke points, since they might belong to another 
	// document
	return *this;
}

void
Selection::addStroke(const Page& page, const Stroke& stroke) {

	// to detach the stroke from the page, we have to shift it by the page 
	// position
	Stroke selectionCopy = stroke;
	selectionCopy.shift(page.getShift());

	fitBoundingBox(selectionCopy.getBoundingBox());

	add(selectionCopy);
}

void
Selection::anchor(Document& document) {

	for (unsigned int i = 0; i < numStrokes(); i++) {

		Stroke& stroke = getStroke(i);

		LOG_DEBUG(selectionlog) << "anchoring stroke " << i << " at " << stroke.getShift() << " in selection at " << getShift() << std::endl;

		// correct for the transformation applied to the selection
		stroke.scale(getScale());
		stroke.shift(getShift());

		// get the page that is closest to the center of the stroke
		unsigned int p = document.getPageIndex(stroke.getBoundingBox().center());
		Page& page = document.getPage(p);

		LOG_DEBUG(selectionlog) << "page " << p << " is closest" << std::endl;

		// correct for the position of the page
		stroke.shift(-page.getShift());

		LOG_DEBUG(selectionlog) << "relative to page, stroke is now at " << stroke.getShift() << std::endl;

		// add it
		document.getPage(p).addStroke(stroke);
	}
}
