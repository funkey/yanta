#ifndef STROKES_H__
#define STROKES_H__

#include <pipeline/Data.h>
#include "Stroke.h"
#include "StrokePoint.h"
#include "StrokePoints.h"

class Strokes : public pipeline::Data {

public:

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

		_strokes.push_back(Stroke(begin));
	}

	/**
	 * Get the current stroke.
	 */
	inline Stroke& currentStroke() { return _strokes.back(); }
	const inline Stroke& currentStroke() const { return _strokes.back(); }

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
	util::rect<double> erase(const util::point<double>& position, double radius);

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
	 * Get the list of all stroke points.
	 */
	inline StrokePoints& getStrokePoints() { return _strokePoints; }
	inline const StrokePoints& getStrokePoints() const { return _strokePoints; }

private:

	/**
	 * Erase points from a stroke by splitting. Reports the changed area.
	 */
	util::rect<double> erase(
			Stroke* stroke,
			const util::point<double>& position,
			double radius);

	bool intersectsErasorCircle(
			const util::point<double> lineStart,
			const util::point<double> lineEnd,
			const util::point<double> center,
			double radius2);

	// global list of stroke points
	StrokePoints _strokePoints;

	std::vector<Stroke> _strokes;
};

#endif // STROKES_H__

