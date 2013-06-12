#ifndef STROKE_H__
#define STROKE_H__

#include <vector>
#include <util/point.hpp>
#include <util/rect.hpp>

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

class Stroke {

public:

	Stroke() :
		_width(1.0),
		_boundingBox(0, 0, 0, 0),
		_finished(false) {}

	void add(const StrokePoint& point) {

		if (_points.size() == 0) {

			_boundingBox.minX = point.position.x - _width;
			_boundingBox.minY = point.position.y - _width;
			_boundingBox.maxX = point.position.x + _width;
			_boundingBox.maxY = point.position.y + _width;

		} else {

			_boundingBox.minX = std::min(_boundingBox.minX, point.position.x - _width);
			_boundingBox.minY = std::min(_boundingBox.minY, point.position.y - _width);
			_boundingBox.maxX = std::max(_boundingBox.maxX, point.position.x + _width);
			_boundingBox.maxY = std::max(_boundingBox.maxY, point.position.y + _width);
		}

		_points.push_back(point);
	}

	unsigned int size() const {

		return _points.size();
	}

	StrokePoint& operator[](unsigned int i) {

		return _points[i];
	}

	const StrokePoint& operator[](unsigned int i) const {

		return _points[i];
	}

	double width() const {

		return _width;
	}

	const util::rect<double>& boundingBox() const {

		return _boundingBox;
	}

	void finish() {

		_finished = true;
	}

	bool finished() const {

		return _finished;
	}

private:

	// TODO: make this a property of a pen
	double _width;

	util::rect<double> _boundingBox;

	std::vector<StrokePoint> _points;

	bool _finished;
};

#endif // STROKE_H__

