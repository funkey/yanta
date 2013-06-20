#include <util/Logger.h>
#include "Canvas.h"

logger::LogChannel canvaslog("canvaslog", "[Canvas] ");

Canvas::Canvas() {}

Canvas::Canvas(Canvas& other) {

	copyFrom(other);
}

Canvas&
Canvas::operator=(Canvas& other) {

	copyFrom(other);
}

void
Canvas::createPage(
		const util::point<CanvasPrecision>& position,
		const util::point<PagePrecision>&   size) {

	LOG_DEBUG(canvaslog) << "created a new page" << std::endl;

	_pages.push_back(Page(this, position, size));
}

util::rect<CanvasPrecision>
Canvas::erase(const util::point<CanvasPrecision>& begin, const util::point<CanvasPrecision>& end) {

	_currentPage = getPageIndex(begin);

	return _pages[_currentPage].erase(begin, end);
}

util::rect<CanvasPrecision>
Canvas::erase(const util::point<CanvasPrecision>& position, CanvasPrecision radius) {

	_currentPage = getPageIndex(position);

	return _pages[_currentPage].erase(position, radius);
}

void
Canvas::copyFrom(Canvas& other) {

	_strokePoints = other._strokePoints;
	_currentPage  = other._currentPage;

	// We can't just copy pages, since they have a reference to the canvas they 
	// belong to. Therefore, we properly initialize our pages and copy the 
	// relevant parts, only.
	_pages.clear();

	for (unsigned int i = 0; i < other.numPages(); i++) {

		createPage(other.getPage(i).getPosition(), other.getPage(i).getSize());
		_pages[i] = other.getPage(i);
	}
}
