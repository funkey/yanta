#include <fstream>
#include "CanvasReader.h"

CanvasReader::CanvasReader(const std::string& filename) :
	_filename(filename) {

	registerOutput(_canvas, "canvas");
}

void
CanvasReader::updateOutputs() {

	std::ifstream in(_filename.c_str());

	if (!in.good())
		return;

	// read the file version (currently, we only have one)
	unsigned int fileVersion;
	in >> fileVersion;

	readStrokePoints(in);

	unsigned int numStrokes = 0;
	in >> numStrokes;

	for (unsigned int i = 0; i < numStrokes; i++)
		readStroke(in);
}

void
CanvasReader::readStrokePoints(std::ifstream& in) {

	unsigned long numPoints = 0;
	in >> numPoints;

	double x, y, pressure;
	long unsigned int timestamp;

	StrokePoints& points = _canvas->getStrokePoints();

	for (unsigned int i = 0; i < numPoints; i++) {

		in >> x >> y >> pressure >> timestamp;
		points.add(StrokePoint(util::point<double>(x, y), pressure, timestamp));
	}
}

void
CanvasReader::readStroke(std::ifstream& in) {

	unsigned long begin, end;
	double penWidth;
	unsigned char red, green, blue, alpha;

	in >> begin >> end >> penWidth >> red >> green >> blue >> alpha;

	Style style;
	style.setWidth(penWidth);
	style.setColor(red, green, blue, alpha);

	_canvas->getPage(0).createNewStroke(begin);
	_canvas->getPage(0).currentStroke().setEnd(end);
	_canvas->getPage(0).currentStroke().setStyle(style);
	_canvas->getPage(0).currentStroke().finish(_canvas->getStrokePoints());
}
