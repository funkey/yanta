#ifndef YANTA_CANVAS_H__
#define YANTA_CANVAS_H__

#include <pipeline/Data.h>
#include "Precision.h"
#include "Stroke.h"
#include "StrokePoint.h"
#include "StrokePoints.h"
#include "Page.h"

class Canvas : public pipeline::Data {

public:

	Canvas();

	Canvas(Canvas& other);

	Canvas& operator=(Canvas& other);

	void createPage(
			const util::point<CanvasPrecision>& position,
			const util::point<PagePrecision>&   size);

	/**
	 * Create a new stroke starting behind the existing points.
	 */
	inline void createNewStroke(
			const util::point<CanvasPrecision>& start,
			double                              pressure,
			unsigned long                       timestamp) {

		_currentPage = getPageIndex(start);

		_pages[_currentPage].createNewStroke(start, pressure, timestamp);
	}

	/**
	 * Get the current stroke.
	 */
	inline Stroke& currentStroke() { return _pages[_currentPage].currentStroke(); }
	const inline Stroke& currentStroke() const { return _pages[_currentPage].currentStroke(); }

	/**
	 * Add a new stroke point to the global list and append it to the current 
	 * stroke.
	 */
	inline void addStrokePoint(
			const util::point<CanvasPrecision>& position,
			double                              pressure,
			unsigned long                       timestamp) {

		_pages[_currentPage].addStrokePoint(position, pressure, timestamp);
	}

	/**
	 * Finish appending the current stroke and prepare for the next stroke.
	 */
	inline void finishCurrentStroke() {

		_pages[_currentPage].currentStroke().finish(_strokePoints);
	}

	/**
	 * Virtually erase points within the given postion and radius by splitting 
	 * the involved strokes.
	 */
	util::rect<CanvasPrecision> erase(
			const util::point<CanvasPrecision>& position,
			CanvasPrecision                     radius);

	/**
	 * Virtually erase strokes that intersect with the given line by setting 
	 * their end to their start.
	 */
	util::rect<CanvasPrecision> erase(
			const util::point<CanvasPrecision>& begin,
			const util::point<CanvasPrecision>& end);

	/**
	 * Get the list of all stroke points.
	 */
	inline StrokePoints& getStrokePoints() { return _strokePoints; }
	inline const StrokePoints& getStrokePoints() const { return _strokePoints; }

	/**
	 * Get the index of the page that is closest to the given canvas point.
	 */
	inline unsigned int getPageIndex(const util::point<CanvasPrecision>& position) {

		// quickly check if there is a page that contains the point (this should 
		// be true in the vast majority of all cases)
		for (unsigned int i = 0; i < _pages.size(); i++)
			if (_pages[i].getPageBoundingBox().contains(position))
				return i;

		// if we haven't been successfull, let's get the closest page instead
		double minDistance = -1;
		unsigned int closestPage = 0;

		for (unsigned int i = 0; i < _pages.size(); i++) {

			const util::rect<CanvasPrecision>& bb = _pages[i].getPageBoundingBox();

			double l = bb.minX - position.x;
			double r = position.x - bb.maxX;
			double t = bb.minY - position.y;
			double b = position.y - bb.maxY;

			double distance = -1;

			if (l > 0) distance = l;
			if (distance < 0 || (r > 0 && r < distance)) distance = r;
			if (distance < 0 || (t > 0 && t < distance)) distance = t;
			if (distance < 0 || (b > 0 && b < distance)) distance = b;

			if (minDistance < 0 || distance < minDistance) {

				minDistance = distance;
				closestPage = i;
			}
		}

		return closestPage;
	}

	/**
	 * Get a page of the canvas.
	 */
	inline Page& getPage(unsigned int i) { return _pages[i]; }
	inline const Page& getPage(unsigned int i) const { return _pages[i]; }

	/**
	 * Get the number of pages in the canvas.
	 */
	inline unsigned int numPages() const { return _pages.size(); }

	/**
	 * Get the number of strokes of all pages.
	 */
	inline unsigned int numStrokes() const {

		unsigned int n = 0;

		for (unsigned int i = 0; i < numPages(); i++)
			n += _pages[i].numStrokes();

		return n;
	}

private:

	void copyFrom(Canvas& other);

	// global list of stroke points
	StrokePoints _strokePoints;

	// the pages on the canvas
	std::vector<Page> _pages;

	// the number of the current page
	unsigned int _currentPage;
};

#endif // YANTA_CANVAS_H__

