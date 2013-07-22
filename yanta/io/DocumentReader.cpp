#include <fstream>

#include "DocumentReader.h"

DocumentReader::DocumentReader(const std::string& filename) :
	_filename(filename) {

	registerOutput(_document, "document");
}

void
DocumentReader::updateOutputs() {

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
		_document->createPage(util::point<DocumentPrecision>(0, 0), util::point<PagePrecision>(1, 1));

		unsigned int numStrokes = 0;
		in >> numStrokes;

		for (unsigned int i = 0; i < numStrokes; i++)
			readStroke(in, 0, fileVersion);
	}

	if (fileVersion >= 2) {

		unsigned int numPages = 0;
		in >> numPages;

		for (unsigned int i = 0; i < numPages; i++)
			readPage(in, i, fileVersion);
	}
}

void
DocumentReader::readStrokePoints(std::ifstream& in) {

	unsigned long numPoints = 0;
	in >> numPoints;

	double x, y, pressure;
	long unsigned int timestamp;

	StrokePoints& points = _document->getStrokePoints();

	for (unsigned int i = 0; i < numPoints; i++) {

		in >> x >> y >> pressure >> timestamp;
		points.add(StrokePoint(util::point<double>(x, y), pressure, timestamp));
	}
}

void
DocumentReader::readPage(std::ifstream& in, unsigned int page, unsigned int fileVersion) {

	DocumentPrecision positionX, positionY;
	in >> positionX >> positionY;

	double width, height;
	in >> width >> height;

	_document->createPage(util::point<DocumentPrecision>(positionX, positionY), util::point<PagePrecision>(width, height));

	unsigned int numStrokes = 0;
	in >> numStrokes;

	for (unsigned int i = 0; i < numStrokes; i++)
		readStroke(in, page, fileVersion);
}

void
DocumentReader::readStroke(std::ifstream& in, unsigned int page, unsigned int fileVersion) {

	unsigned long begin, end;
	double penWidth;
	unsigned char red, green, blue, alpha;
	util::point<PagePrecision> scale;
	util::point<PagePrecision> shift;

	in >> begin >> end >> penWidth >> red >> green >> blue >> alpha;

	if (fileVersion >= 3)
		in >> scale.x >> scale.y >> shift.x >> shift.y;

	Style style;
	style.setWidth(penWidth);
	style.setColor(red, green, blue, alpha);

	_document->getPage(page).createNewStroke(begin);
	_document->getPage(page).currentStroke().setEnd(end);
	_document->getPage(page).currentStroke().setStyle(style);
	if (fileVersion >= 3) {
		_document->getPage(page).currentStroke().setScale(scale);
		_document->getPage(page).currentStroke().setShift(shift);
	}
	_document->getPage(page).currentStroke().finish(_document->getStrokePoints());
}
