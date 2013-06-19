#ifndef YANTA_CANVAS_H__
#define YANTA_CANVAS_H__

#include <pipeline/Data.h>
#include "Stroke.h"
#include "StrokePoint.h"
#include "StrokePoints.h"

class Canvas : public pipeline::Data {

public:

	/**
	 * The precision in which canvas coordinates are measured.
	 */
	typedef double Precision;

	/**
	 * Create a new stroke starting behind the existing points.
	 */
	inline void createNewStroke() {

		createNewStroke(_strokePoints.size());
	}

	/**
	 * Create a new stroke starting at begin (and without an end).
	 */
	inline void createNewStroke(unsigned long begin) {

		// if this happens in the middle of a draw, finish the unfinished
		if (numStrokes() > 0 && !currentStroke().finished())
			currentStroke().finish(_strokePoints);

		_canvas.push_back(Stroke(begin));
	}

	/**
	 * Get the current stroke.
	 */
	inline Stroke& currentStroke() { return _canvas.back(); }
	const inline Stroke& currentStroke() const { return _canvas.back(); }

	/**
	 * Add a new stroke point to the global list and append it to the current 
	 * stroke.
	 */
	inline void addStrokePoint(const StrokePoint& point) {

		_strokePoints.add(point);
		currentStroke().setEnd(_strokePoints.size());
	}

	/**
	 * Finish appending the current stroke and prepare for the next stroke.
	 */
	inline void finishCurrentStroke() {

		currentStroke().finish(_strokePoints);
	}

	/**
	 * Get the stroke point with the given index.
	 */
	inline StrokePoint& getStrokePoint(unsigned long i) { return _strokePoints[i]; }
	inline const StrokePoint& getStrokePoint(unsigned long i) const { return _strokePoints[i]; }

	/**
	 * Virtually erase points within the given postion and radius by splitting 
	 * the involved strokes.
	 */
	util::rect<Canvas::Precision> erase(const util::point<Canvas::Precision>& position, Canvas::Precision radius);

	/**
	 * Virtually erase strokes that intersect with the given line by setting 
	 * their end to their start.
	 */
	util::rect<Canvas::Precision> erase(const util::point<Canvas::Precision>& begin, const util::point<Canvas::Precision>& end);

	/**
	 * Get a stroke by its index.
	 */
	inline Stroke& getStroke(unsigned int i) { return _canvas[i]; }
	inline const Stroke& getStroke(unsigned int i) const { return _canvas[i]; }

	/**
	 * Get the number of strokes.
	 */
	inline unsigned int numStrokes() const {

		return _canvas.size();
	}

	/**
	 * Get the list of all stroke points.
	 */
	inline StrokePoints& getStrokePoints() { return _strokePoints; }
	inline const StrokePoints& getStrokePoints() const { return _strokePoints; }

private:

	/**
	 * Erase points from a stroke by splitting. Reports the changed area.
	 */
	util::rect<Canvas::Precision> erase(
			Stroke* stroke,
			const util::point<Canvas::Precision>& position,
			Canvas::Precision radius);

	/**
	 * Erase the given stroke if it intersects the line.
	 */
	util::rect<Canvas::Precision> erase(
			Stroke* stroke,
			const util::point<Canvas::Precision>& lineBegin,
			const util::point<Canvas::Precision>& lineEnd);

	bool intersectsErasorCircle(
			const util::point<Canvas::Precision> lineStart,
			const util::point<Canvas::Precision> lineEnd,
			const util::point<Canvas::Precision> center,
			Canvas::Precision radius2);

	/**
	 * Test, whether the lines p + t*r and q + u*s, with t and u in [0,1], 
	 * intersect.
	 */
	bool intersectLines(
			const util::point<Canvas::Precision>& p,
			const util::point<Canvas::Precision>& r,
			const util::point<Canvas::Precision>& q,
			const util::point<Canvas::Precision>& s);

	// global list of stroke points
	StrokePoints _strokePoints;

	std::vector<Stroke> _canvas;
};

#endif // YANTA_CANVAS_H__

