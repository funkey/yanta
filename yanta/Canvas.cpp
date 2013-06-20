#include <util/Logger.h>
#include "Canvas.h"

logger::LogChannel canvaslog("canvaslog", "[Canvas] ");

Canvas::Canvas() {

	createPage(
			util::point<CanvasPrecision>(0.0, 0.0),
			util::point<PagePrecision>(100.0, 100.0));
}

void
Canvas::createPage(
		const util::point<CanvasPrecision>& position,
		const util::point<PagePrecision>&   size) {

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

