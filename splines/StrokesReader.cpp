#include <fstream>
#include "StrokesReader.h"

StrokesReader::StrokesReader(const std::string& filename) :
	_filename(filename) {

	registerOutput(_strokes, "strokes");
}

void
StrokesReader::updateOutputs() {

	std::ifstream in(_filename.c_str());

	if (!in.good())
		return;

	// read the file version (currently, we only have one)
	unsigned int fileVersion;
	in >> fileVersion;

	unsigned int numStrokes;
	in >> numStrokes;

	for (unsigned int i = 0; i < numStrokes; i++)
		readStroke(in);
}

void
StrokesReader::readStroke(std::ifstream& in) {

	Stroke stroke;

	unsigned int numPoints;
	in >> numPoints;

	double x, y, pressure;
	long unsigned int timestamp;

	for (unsigned int i = 0; i < numPoints; i++) {

		in >> x >> y >> pressure >> timestamp;
		stroke.push_back(StrokePoint(util::point<double>(x, y), pressure, timestamp));
	}

	_strokes->add(stroke);
}
