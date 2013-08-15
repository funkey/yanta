#ifndef YANTA_TRANSFORMABLE_H__
#define YANTA_TRANSFORMABLE_H__

#include <util/point.hpp>
#include <util/rect.hpp>
#include <util/Transformation.h>

template <typename Precision>
class Transformable {

public:

	Transformable() :
		_boundingBox(0, 0, 0, 0) {}

	/**
	 * Get the shift of this object.
	 */
	inline const util::point<Precision>& getShift() const {

		return _transformation.getShift();
	}

	/**
	 * Get the scale of this object.
	 */
	inline const util::point<Precision>& getScale() const {

		return _transformation.getScale();
	}

	/**
	 * Get the transformation of this object.
	 */
	inline const Transformation<Precision>& getTransformation() const {

		return _transformation;
	}

	/**
	 * Set the shift of this object.
	 */
	inline void setShift(const util::point<Precision>& shift) {

		_boundingBox += shift - _transformation.getShift();
		_transformation.setShift(shift);
	}

	/**
	 * Set the scale of this object.
	 */
	inline void setScale(const util::point<Precision>& scale) {

		_boundingBox *= scale/_transformation.getScale();
		_transformation.setScale(scale);
	}

	/**
	 * Shift this object.
	 */
	inline void shift(const util::point<Precision>& shift) {

		_transformation.shift(shift);
		_boundingBox += shift;
	}

	/**
	 * Scale this object.
	 */
	inline void scale(const util::point<Precision>& scale) {

		_transformation.scale(scale);
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
			_boundingBox = r*_transformation.getScale() + _transformation.getShift();
		else
			_boundingBox.fit(r*_transformation.getScale() + _transformation.getShift());
	}

	/**
	 * Change the bounding box to fit the given point after scaling and 
	 * shifting.
	 */
	void fitBoundingBox(const util::point<Precision>& p) {

		if (_boundingBox.isZero())
			_boundingBox = util::rect<Precision>(p.x, p.y, p.x, p.y)*_transformation.getScale() + _transformation.getShift();
		else
			_boundingBox.fit(p*_transformation.getScale() + _transformation.getShift());
	}

	/**
	 * Get the bounding box of this object, accordingly scaled and shifted.
	 */
	const util::rect<Precision>& getBoundingBox() const {

		return _boundingBox;
	}

private:

	Transformation<Precision> _transformation;

	util::rect<Precision> _boundingBox;
};


#endif // YANTA_TRANSFORMABLE_H__

