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

	// no pages in version 1
	if (fileVersion == 1) {

		// create a dummy page
		_canvas->createPage(util::point<CanvasPrecision>(0, 0), util::point<PagePrecision>(1, 1));

		unsigned int numStrokes = 0;
		in >> numStrokes;

		for (unsigned int i = 0; i < numStrokes; i++)
			readStroke(in, 0);
	}

	if (fileVersion == 2) {

		unsigned int numPages = 0;
		in >> numPages;

		for (unsigned int i = 0; i < numPages; i++)
			readPage(in, i);
	}
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
CanvasReader::readPage(std::ifstream& in, unsigned int page) {

	CanvasPrecision positionX, positionY;
	in >> positionX >> positionY;

	double width, height;
	in >> width >> height;

	_canvas->createPage(util::point<CanvasPrecision>(positionX, positionY), util::point<PagePrecision>(width, height));

	unsigned int numStrokes = 0;
	in >> numStrokes;

	for (unsigned int i = 0; i < numStrokes; i++)
		readStroke(in, page);
}

void
CanvasReader::readStroke(std::ifstream& in, unsigned int page) {

	unsigned long begin, end;
	double penWidth;
	unsigned char red, green, blue, alpha;

	in >> begin >> end >> penWidth >> red >> green >> blue >> alpha;

	Style style;
	style.setWidth(penWidth);
	style.setColor(red, green, blue, alpha);

	_canvas->getPage(page).createNewStroke(begin);
	_canvas->getPage(page).currentStroke().setEnd(end);
	_canvas->getPage(page).currentStroke().setStyle(style);
	_canvas->getPage(page).currentStroke().finish(_canvas->getStrokePoints());
}