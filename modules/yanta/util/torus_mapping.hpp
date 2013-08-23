#ifndef YANTA_UTIL_TORUS_MAPPING_H__
#define YANTA_UTIL_TORUS_MAPPING_H__

#include <util/point.hpp>
#include <util/rect.hpp>

#include "ring_mapping.hpp"

/**
 * Provides a mapping from logical coordinates in a 2D interval of size wxh to 
 * physical coordinates in [0,w)x[0,h). When the logical interval of gets 
 * shifted, most of the mapping stays as it is, only the boundary of the mapping 
 * changes, thus forming a torus.
 */
template <typename Precision, int W = util::detail::Dynamic, int H = util::detail::Dynamic>
class torus_mapping {

public:

	/**
	 * Create a new mapping from [0,w)x[0,h) to [0,w)x[0,h).
	 */
	torus_mapping(Precision w = util::detail::Dynamic, Precision h = util::detail::Dynamic) :
		_xring(w),
		_yring(h) {}

	/**
	 * Get the logical region represented by this mapping.
	 */
	inline util::rect<Precision> get_region() {

		Precision bx, by, ex, ey;

		_xring.get_interval(bx, ex);
		_yring.get_interval(by, ey);

		return util::rect<Precision>(bx, by, ex, ey);
	}

	/**
	 * Map a logical point p to its physical coordinats.
	 */
	inline util::point<Precision> map(const util::point<Precision>& p) {

		return util::point<Precision>(_xring.map(p.x), _yring.map(p.y));
	}

	/**
	 * Shift the interval represented by this torus by s.
	 */
	void shift(const util::point<Precision>& s) {

		_xring.shift(s.x);
		_yring.shift(s.y);
	}

	/**
	 * Reset the mapping, such that [begin.x,begin.x+w)x[begin.y,begin.y+h) maps 
	 * to [0,w)x[0,h).
	 */
	void reset(const util::point<Precision>& begin) {

		_xring.reset(begin.x);
		_yring.reset(begin.y);
	}

	/**
	 * Split a logical 2D interval into four logical intervals, such that the 
	 * mapping in these intervals is linear (i.e., does not wrap around the 
	 * torus boundaries).
	 *
	 * Up to three of the intervals can be empyt, in which case it is set to 
	 * [0,0,0,0).
	 *
	 * @param region
	 *              The 2D interval to split.
	 * @param subregions [out]
	 *              Start of an array of four rectangles to be filled with the 
	 *              four subintervals.
	 */
	void split(const util::rect<Precision>& region, util::rect<Precision>* subregions) {

		Precision xbegins[2];
		Precision xends[2];
		Precision ybegins[2];
		Precision yends[2];

		_xring.split(region.minX, region.maxX, xbegins, xends);
		_yring.split(region.minY, region.maxY, ybegins, yends);

		subregions[0].minX = xbegins[0];
		subregions[0].minY = ybegins[0];
		subregions[0].maxX = xends[0];
		subregions[0].maxY = yends[0];

		subregions[1].minX = xbegins[1];
		subregions[1].minY = ybegins[0];
		subregions[1].maxX = xends[1];
		subregions[1].maxY = yends[0];

		subregions[2].minX = xbegins[0];
		subregions[2].minY = ybegins[1];
		subregions[2].maxX = xends[0];
		subregions[2].maxY = yends[1];

		subregions[3].minX = xbegins[1];
		subregions[3].minY = ybegins[1];
		subregions[3].maxX = xends[1];
		subregions[3].maxY = yends[1];

		for (int i = 0; i < 4; i++)
			if (subregions[i].area() <= 0) {

				subregions[i].minX = 0;
				subregions[i].maxX = 0;
				subregions[i].minY = 0;
				subregions[i].maxY = 0;
			}
	}

private:

	ring_mapping<Precision, W> _xring;
	ring_mapping<Precision, H> _yring;
};

#endif // YANTA_UTIL_TORUS_MAPPING_H__

