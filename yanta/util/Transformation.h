#ifndef YANTA_UTIL_TRANSFORMATION_H__
#define YANTA_UTIL_TRANSFORMATION_H__

template <typename Precision>
class Transformation {

public:

	Transformation() :
		_shift(0, 0),
		_scale(1, 1) {}

	/**
	 * Get the shift of this transformation.
	 */
	inline const util::point<Precision>& getShift() const {

		return _shift;
	}

	/**
	 * Get the scale of this transformation.
	 */
	inline const util::point<Precision>& getScale() const {

		return _scale;
	}

	inline Transformation<Precision> getInverse() const {

		// p = q*s + t
		//
		// q = (p - t)/s = p*1/s + (-t/s)

		Transformation<Precision> inverse;
		inverse.setScale(util::point<Precision>(1.0/_scale.x, 1.0/_scale.y));
		inverse.setShift(-_shift/_scale);

		return inverse;
	}

	inline Transformation<Precision> applyTo(const Transformation<Precision>& other) const {

		Transformation<Precision> result;

		// p = (q*s1 + t1)*s2 + t2 = q*s1*s2 + (t1*s2 + t2)

		result.setScale(other._scale*_scale);
		result.setShift(other._shift*_scale + _shift);

		return result;
	}

	inline util::rect<Precision> applyTo(const util::rect<Precision>& rect) const {

		return rect*_scale + _shift;
	}

	inline util::point<Precision> applyTo(const util::point<Precision>& point) const {

		return point*_scale + _shift;
	}

	/**
	 * Set the shift of this transformation.
	 */
	inline void setShift(const util::point<Precision>& shift) {

		_shift = shift;
	}

	/**
	 * Set the scale of this transformation.
	 */
	inline void setScale(const util::point<Precision>& scale) {

		_scale = scale;
	}

	/**
	 * Shift this transformation.
	 */
	inline void shift(const util::point<Precision>& shift) {

		_shift += shift;
	}

	/**
	 * Scale this transformation.
	 */
	inline void scale(const util::point<Precision>& scale) {

		_scale *= scale;
	}


private:

	util::point<Precision> _shift;
	util::point<Precision> _scale;
};

template <typename Precision>
std::ostream& operator<<(std::ostream& out, const Transformation<Precision>& transformation) {

	return out << "[ s: " << transformation.getScale() << ", t: " << transformation.getShift() << "]";
}

#endif // YANTA_UTIL_TRANSFORMATION_H__

