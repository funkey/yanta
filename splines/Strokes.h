#ifndef STROKES_H__
#define STROKES_H__

#include <pipeline/Data.h>
#include "Stroke.h"

class Strokes : public pipeline::Data {

public:

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

	Stroke& operator[](unsigned int i) {

		return _strokes[i];
	}

	const Stroke& operator[](unsigned int i) const {

		return _strokes[i];
	}

	unsigned int size() const {

		return _strokes.size();
	}

private:

	std::vector<Stroke> _strokes;
};

#endif // STROKES_H__

