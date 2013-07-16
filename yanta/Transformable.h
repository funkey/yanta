#ifndef YANTA_TRANSFORMABLE_H__
#define YANTA_TRANSFORMABLE_H__

#include <util/point.hpp>
#include <util/rect.hpp>

template <typename Precision>
class Transformable {

public:

	Transformable() :
		_shift(0, 0),
		_scale(1, 1),
		_boundingBox(0, 0, 0, 0) {}

	/**
	 * Get the shift of this object.
	 */
	inline const util::point<CanvasPrecision>& getShift() const {

		return _shift;
	}

	/**
	 * Get the scale of this object.
	 */
	inline const util::point<CanvasPrecision>& getScale() const {

		return _scale;
	}

	/**
	 * Get the shift of this object.
	 */
	inline void setShift(const util::point<CanvasPrecision>& shift) const {

		_boundingBox += shift - _shift;
		_shift = shift;
	}

	/**
	 * Get the scale of this object.
	 */
	inline void setScale(const util::point<CanvasPrecision>& scale) const {

		_boundingBox *= scale/_scale;
		_scale = scale;
	}

	/**
	 * Shift this object.
	 */
	inline void shift(const util::point<CanvasPrecision>& shift) {

		_shift += shift;
		_boundingBox += shift;
	}

	/**
	 * Scale this object.
	 */
	inline void scale(const util::point<CanvasPrecision>& scale) {

		_scale *= scale;
		_boundingBox *= scale;
	}

	/**
	 * Set the bounding box to be empty.
	 */
	void resetBoundingBox() {

		_boundingBox = util::rect<Precision>(0, 0, 0, 0);
	}

	/**
	 * Change the bounding box to fit the given area after scaling and shifting.
	 */
	void fitBoundingBox(const util::rect<Precision>& r) {

		if (_boundingBox.isZero())
			_boundingBox = r*_scale + _shift;
		else
			_boundingBox.fit(r*_scale + _shift);
	}

	/**
	 * Change the bounding box to fit the given point after scaling and 
	 * shifting.
	 */
	void fitBoundingBox(const util::point<Precision>& p) {

		if (_boundingBox.isZero())
			_boundingBox = util::rect<Precision>(p.x, p.y, p.x, p.y)*_scale + _shift;
		else
			_boundingBox.fit(p*_scale + _shift);
	}

	/**
	 * Get the bounding box of this object, accordingly scaled and shifted.
	 */
	const util::rect<Precision>& getBoundingBox() const {

		return _boundingBox;
	}

private:

	util::point<Precision> _shift;
	util::point<Precision> _scale;

	util::rect<Precision> _boundingBox;
};


#endif // YANTA_TRANSFORMABLE_H__

