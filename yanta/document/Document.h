#ifndef YANTA_DOCUMENT_H__
#define YANTA_DOCUMENT_H__

#include <pipeline/Data.h>
#include "Precision.h"
#include "Stroke.h"
#include "StrokePoint.h"
#include "StrokePoints.h"
#include "Page.h"

class Document : public pipeline::Data {

public:

	Document();

	Document(Document& other);

	Document& operator=(Document& other);

	void createPage(
			const util::point<DocumentPrecision>& position,
			const util::point<PagePrecision>&   size);

	/**
	 * Create a new stroke starting behind the existing points.
	 */
	inline void createNewStroke(
			const util::point<DocumentPrecision>& start,
			double                              pressure,
			unsigned long                       timestamp) {

		_currentPage = getPageIndex(start);

		_pages[_currentPage].createNewStroke(start, pressure, timestamp);
	}

	/**
	 * Set the style of the current stroke.
	 */
	inline void setCurrentStrokeStyle(const Style& style) {

		_pages[_currentPage].currentStroke().setStyle(style);
	}

	/**
	 * Add a new stroke point to the global list and append it to the current 
	 * stroke.
	 */
	inline void addStrokePoint(
			const util::point<DocumentPrecision>& position,
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
	 * Check if there is currently a stroke that has not been finished yet 
	 * (i.e., the pen still touches the paper).
	 */
	inline bool hasOpenStroke() const {

		if (_pages[_currentPage].numStrokes() > 0 && !_pages[_currentPage].currentStroke().finished())
			return true;

		return false;
	}

	/**
	 * Virtually erase points within the given postion and radius by splitting 
	 * the involved strokes.
	 */
	util::rect<DocumentPrecision> erase(
			const util::point<DocumentPrecision>& position,
			DocumentPrecision                     radius);

	/**
	 * Virtually erase strokes that intersect with the given line by setting 
	 * their end to their start.
	 */
	util::rect<DocumentPrecision> erase(
			const util::point<DocumentPrecision>& begin,
			const util::point<DocumentPrecision>& end);

	/**
	 * Get the list of all stroke points.
	 */
	inline StrokePoints& getStrokePoints() { return _strokePoints; }
	inline const StrokePoints& getStrokePoints() const { return _strokePoints; }

	/**
	 * Get the index of the page that is closest to the given document point.
	 */
	inline unsigned int getPageIndex(const util::point<DocumentPrecision>& position) {

		// quickly check if there is a page that contains the point (this should 
		// be true in the vast majority of all cases)
		for (unsigned int i = 0; i < _pages.size(); i++)
			if (_pages[i].getPageBoundingBox().contains(position))
				return i;

		// if we haven't been successfull, let's get the closest page instead
		double minDistance = -1;
		unsigned int closestPage = 0;

		for (unsigned int i = 0; i < _pages.size(); i++) {

			const util::rect<DocumentPrecision>& bb = _pages[i].getPageBoundingBox();

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
	 * Get a page of the document.
	 */
	inline Page& getPage(unsigned int i) { return _pages[i]; }
	inline const Page& getPage(unsigned int i) const { return _pages[i]; }

	/**
	 * Get the number of pages in the document.
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

	void copyFrom(Document& other);

	// global list of stroke points
	StrokePoints _strokePoints;

	// the pages on the document
	std::vector<Page> _pages;

	// the number of the current page
	unsigned int _currentPage;
};

#endif // YANTA_DOCUMENT_H__

