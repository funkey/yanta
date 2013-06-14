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

	readStrokePoints(in);

	unsigned int numStrokes;
	in >> numStrokes;

	for (unsigned int i = 0; i < numStrokes; i++)
		readStroke(in);
}

void
StrokesReader::readStrokePoints(std::ifstream& in) {

	unsigned long numPoints;
	in >> numPoints;

	double x, y, pressure;
	long unsigned int timestamp;

	StrokePoints& points = _strokes->getStrokePoints();

	for (unsigned int i = 0; i < numPoints; i++) {

		in >> x >> y >> pressure >> timestamp;
		points.add(StrokePoint(util::point<double>(x, y), pressure, timestamp));
	}
}

void
StrokesReader::readStroke(std::ifstream& in) {

	unsigned long begin, end;
	double penWidth;

	in >> begin >> end >> penWidth;

	Pen pen;
	pen.setWidth(penWidth);

	_strokes->createNewStroke();
	_strokes->currentStroke().setBegin(begin);
	_strokes->currentStroke().setEnd(end);
	_strokes->currentStroke().setPen(pen);
	_strokes->currentStroke().finish(_strokes->getStrokePoints());
}
