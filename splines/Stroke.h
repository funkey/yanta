#ifndef STROKE_H__
#define STROKE_H__

#include <vector>
#include <util/point.hpp>
#include <util/rect.hpp>

#include "StrokePoints.h"
#include "Pen.h"

class Stroke {

public:

	Stroke(unsigned long begin = 0) :
		_boundingBox(0, 0, 0, 0),
		_finished(false),
		_begin(begin),
		_end(0) {}

	/**
	 * Get the number of stroke points in this stroke.
	 */
	inline unsigned long size() const {

		if (_end <= _begin)
			return 0;

		return _end - _begin;
	}

	/**
	 * Set the pen this stroke should be drawn with.
	 */
	inline void setPen(const Pen& pen) { _pen = pen; }

	/**
	 * Get the pen this stroke is supposed to be drawn with.
	 */
	inline const Pen& getPen() const {

		return _pen;
	}

	/**
	 * Get the pen this stroke is supposed to be drawn with.
	 */
	inline Pen& getPen() {

		return _pen;
	}

	/**
	 * Get the bounding box of this stroke. Note that not-finished strokes don't 
	 * have a valid bounding box.
	 */
	inline const util::rect<double>& boundingBox() const {

		return _boundingBox;
	}

	/**
	 * Set the first stroke point of this stroke.
	 */
	inline void setBegin(unsigned long index) {

		_begin = index;
	}

	/**
	 * Set the last (exclusive) stroke point of this stroke.
	 */
	inline void setEnd(unsigned long index) {

		_end = index;
	}

	/**
	 * Get the index of the first point of this stroke.
	 */
	inline unsigned long begin() const { return _begin; }

	/**
	 * Get the index of the point after the last point of this stroke.
	 */
	inline unsigned long end() const { return _end; }

	/**
	 * Finish this stroke. Computes the bounding box.
	 */
	inline void finish(const StrokePoints& points) {

		_finished = true;

		computeBoundingBox(points);
	}

	/**
	 * Test, whether this stroke was finished already.
	 */
	inline bool finished() const {

		return _finished;
	}

private:

	void computeBoundingBox(const StrokePoints& points);

	Pen _pen;

	util::rect<double> _boundingBox;

	bool _finished;

	// indices of the stroke points in the global point list
	unsigned long _begin;
	unsigned long _end;
};

#endif // STROKE_H__

