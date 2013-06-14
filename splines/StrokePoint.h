#ifndef SPLINES_STROKE_POINT_H__
#define SPLINES_STROKE_POINT_H__

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

#endif // SPLINES_STROKE_POINT_H__

