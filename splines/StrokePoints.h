#ifndef SPLINES_STROKE_POINTS_H__
#define SPLINES_STROKE_POINTS_H__

#include <vector>
#include <boost/thread/shared_mutex.hpp>

#include "StrokePoint.h"

class StrokePoints {

	typedef std::vector<StrokePoint> points_t;

public:

	StrokePoints() { init(); }

	StrokePoints(StrokePoints& other) { init(); copyFrom(other); }

	StrokePoints& operator=(StrokePoints& other) { copyFrom(other); return *this; }

	/**
	 * Get the ith stroke point.
	 */
	inline StrokePoint& operator[](unsigned long i) { return _points[i]; }
	inline const StrokePoint& operator[](unsigned long i) const { return _points[i]; }

	/**
	 * Get the number of stroke points.
	 */
	inline unsigned long size() const { return _points.size(); }

	/**
	 * Add a new stroke point. Will uniquely lock the mutex, if a reallocation 
	 * is necessary. This method itself is not thread safe.
	 */
	inline void add(const StrokePoint& point) {

		if (_points.size() + 1 <= _points.capacity())

			_points.push_back(point);

		else {

			boost::unique_lock<boost::shared_mutex> lock(_mutex);

			_points.push_back(point);
		}
	}

	/**
	 * Get the shared mutex for the stroke points. Protect all your reading 
	 * access to the points with this mutex.
	 */
	inline boost::shared_mutex& getMutex() { return _mutex; }

private:

	void init() {

		// we will certainly need a lot of them
		_points.reserve(10000);
	}

	void copyFrom(StrokePoints& other) {

		boost::shared_lock<boost::shared_mutex> lockThem(other._mutex);
		boost::unique_lock<boost::shared_mutex> lockMe(_mutex);

		_points = other._points;
	}

	boost::shared_mutex _mutex;
	points_t            _points;

};

#endif // SPLINES_STROKE_POINTS_H__

