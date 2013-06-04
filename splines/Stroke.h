#ifndef STROKE_H__
#define STROKE_H__

#include <vector>
#include <util/point.hpp>

struct StrokePoint {

	StrokePoint(
			util::point<double> position_,
			double pressure_,
			unsigned long timestamp_) :
		position(position_),
		pressure(pressure_),
		timestamp(timestamp_) {}

	util::point<double> position;
	double              pressure;
	unsigned long       timestamp;
};

typedef std::vector<StrokePoint> Stroke;

#endif // STROKE_H__

