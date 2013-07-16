#ifndef STROKE_H__
#define STROKE_H__

#include <vector>
#include <util/point.hpp>
#include <util/rect.hpp>

#include "Precision.h"
#include "StrokePoints.h"
#include "Style.h"

class Stroke {

public:

	Stroke(unsigned long begin = 0) :
		_shift(0, 0),
		_scale(1, 1),
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
	 * Set the style this stroke should be drawn with.
	 */
	inline void setStyle(const Style& style) { _style = style; }

	/**
	 * Get the style this stroke is supposed to be drawn with.
	 */
	inline const Style& getStyle() const {

		return _style;
	}

	/**
	 * Get the style this stroke is supposed to be drawn with.
	 */
	inline Style& getStyle() {

		return _style;
	}

	/**
	 * Get the shift of this stroke that is applied to the coordinates in the 
	 * global stroke points list.
	 */
	inline const util::point<CanvasPrecision>& getShift() const {

		return _shift;
	}

	/**
	 * Get the scale of this stroke that is applied to the coordinats in the 
	 * global stroke points list.
	 */
	inline const util::point<CanvasPrecision>& getScale() const {

		return _scale;
	}

	/**
	 * Shift this stroke by changing its internal transformation. The stroke 
	 * points will remain untouched.
	 */
	inline void shift(const util::point<CanvasPrecision>& shift) {

		_shift += shift;
	}

	/**
	 * Scale this stroke by changing its internal transformation. The stroke 
	 * points will remain untouched.
	 */
	inline void scale(const util::point<CanvasPrecision>& scale) {

		_scale *= scale;
	}

	/**
	 * Get the bounding box of this stroke. Note that not-finished canvas don't 
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

	Style _style;

	util::point<CanvasPrecision> _shift;
	util::point<CanvasPrecision> _scale;
	util::rect<CanvasPrecision>  _boundingBox;

	bool _finished;

	// indices of the stroke points in the global point list
	unsigned long _begin;
	unsigned long _end;
};

#endif // STROKE_H__

