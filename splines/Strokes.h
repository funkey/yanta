#ifndef STROKES_H__
#define STROKES_H__

#include <pipeline/Data.h>
#include "Stroke.h"
#include "StrokePoint.h"

class Strokes : public pipeline::Data {

public:

	Strokes();

	void add(const Stroke& stroke) {

		_strokes.push_back(stroke);
	}

	void createNewStroke() {

		_strokes.push_back(Stroke());
	}

	Stroke& currentStroke() {

		if (_strokes.size() == 0)
			createNewStroke();

		return _strokes.back();
	}

	void finishCurrentStroke() {

		currentStroke().finish();
	}

	util::rect<double> erase(const util::point<double>& position, double radius);

	inline Stroke& operator[](unsigned int i) {

		return _strokes[i];
	}

	inline const Stroke& operator[](unsigned int i) const {

		return _strokes[i];
	}

	inline unsigned int size() const {

		return _strokes.size();
	}

private:

	// global list of stroke points
	std::vector<StrokePoint> _strokePoints;

	std::vector<Stroke> _strokes;
};

#endif // STROKES_H__

