#ifndef YANTA_PAGE_H__
#define YANTA_PAGE_H__

#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "Precision.h"
#include "Stroke.h"
#include "StrokePoints.h"

// forward declaration
class Canvas;

class Page {

public:

	Page(
			Canvas* canvas,
			const util::point<CanvasPrecision>& position,
			const util::point<PagePrecision>&   size);

	Page& operator=(const Page& other);

	/**
	 * Get the position of the space on the canvas.
	 */
	inline const util::point<CanvasPrecision>& getPosition() const { return _position; }

	/**
	 * Get the bounding box of the page, irrespective of its content.
	 */
	inline const util::rect<CanvasPrecision>& getPageBoundingBox() const { return _pageBoundingBox; }

	/**
	 * Get the size of the page, irrespective of its content.
	 */
	inline const util::point<PagePrecision>& getSize() const { return _size; }

	/**
	 * Create a new stroke starting behind the existing points.
	 */
	void createNewStroke(
			const util::point<CanvasPrecision>& start,
			double                              pressure,
			unsigned long                       timestamp);

	/**
	 * Create a new stroke starting with stroke point 'begin' (and without an 
	 * end, yet).
	 */
	void createNewStroke(unsigned long begin);

	/**
	 * Add a complete stroke to this page.
	 */
	void addStroke(const Stroke& stroke) { _strokes.push_back(stroke); }

	/**
	 * Add a stroke point to the current stroke. This appends the stroke point 
	 * to the global list of stroke points.
	 */
	inline void addStrokePoint(
			const util::point<CanvasPrecision>& position,
			double                                pressure,
			unsigned long                         timestamp) {

		// position is in canvas units -- transform it into page units and store 
		// it in global stroke points list
		util::point<PagePrecision> p = position - _position;

		_strokePoints.add(StrokePoint(p, pressure, timestamp));
		currentStroke().setEnd(_strokePoints.size());
	}

	/**
	 * Get a stroke by its index.
	 */
	inline Stroke& getStroke(unsigned int i) { return _strokes[i]; }
	inline const Stroke& getStroke(unsigned int i) const { return _strokes[i]; }

	/**
	 * Get the number of strokes.
	 */
	inline unsigned int numStrokes() const {

		return _strokes.size();
	}

	/**
	 * Get the current stroke of this page.
	 */
	Stroke& currentStroke() { return _strokes.back(); }
	const Stroke& currentStroke() const { return _strokes.back(); }

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
	 * Remove all the strokes from this page for which the given unary predicate 
	 * evaluates to true.
	 *
	 * @return The removed strokes.
	 */
	template <typename Predicate>
	std::vector<Stroke> removeStrokes(Predicate pred) {

		std::vector<Stroke>::iterator newEnd = std::partition(_strokes.begin(), _strokes.end(), !boost::bind(pred, boost::lambda::_1));

		std::vector<Stroke> removed(newEnd, _strokes.end());
		_strokes.resize(newEnd - _strokes.begin());

		return removed;
	}

private:

	/**
	 * Erase points from a stroke by splitting. Reports the changed area.
	 */
	util::rect<PagePrecision> erase(
			Stroke* stroke,
			const util::point<PagePrecision>& position,
			PagePrecision radius);

	/**
	 * Erase the given stroke if it intersects the line.
	 */
	util::rect<PagePrecision> erase(
			Stroke* stroke,
			const util::point<PagePrecision>& lineBegin,
			const util::point<PagePrecision>& lineEnd);

	bool intersectsErasorCircle(
			const util::point<PagePrecision> lineStart,
			const util::point<PagePrecision> lineEnd,
			const util::point<PagePrecision> center,
			PagePrecision radius2);

	/**
	 * Test, whether the lines p + t*r and q + u*s, with t and u in [0,1], 
	 * intersect.
	 */
	bool intersectLines(
			const util::point<PagePrecision>& p,
			const util::point<PagePrecision>& r,
			const util::point<PagePrecision>& q,
			const util::point<PagePrecision>& s);

	inline util::rect<CanvasPrecision> toCanvasCoordinates(const util::rect<PagePrecision>& r) {

		util::rect<CanvasPrecision> result = r;
		return result + _position;
	}

	inline util::point<CanvasPrecision> toCanvasCoordinates(const util::point<PagePrecision>& p) {

		return _position + p;
	}

	inline util::rect<PagePrecision> toPageCoordinates(const util::rect<CanvasPrecision>& r) {

		return r - _position;
	}

	inline util::point<PagePrecision> toPageCoordinates(const util::point<CanvasPrecision>& p) {

		return p - _position;
	}


	// the position and size of this page on the canvas
	util::point<CanvasPrecision> _position;
	util::point<PagePrecision>   _size;

	// the bounding box of the page (not its content) in canvas units
	util::rect<CanvasPrecision> _pageBoundingBox;

	// the global list of stroke points
	StrokePoints& _strokePoints;

	// the strokes on this page
	std::vector<Stroke> _strokes;
};

#endif // YANTA_PAGE_H__

