#include <fstream>
#include "StrokesWriter.h"

StrokesWriter::StrokesWriter(const std::string& filename) :
	_filename(filename) {

	registerInput(_strokes, "strokes");
}

void
StrokesWriter::write() {

	std::ofstream out(_filename.c_str());

	// write the file version (currently, we only have one)
	out << 1 << std::endl;;

	unsigned int numStrokes = _strokes->size();
	out << numStrokes << std::endl;;

	for (unsigned int i = 0; i < numStrokes; i++)
		writeStroke(out, (*_strokes)[i]);
}

void
StrokesWriter::writeStroke(std::ofstream& out, const Stroke& stroke) {

	unsigned int numPoints = stroke.size();
	out << numPoints;

	for (unsigned int i = 0; i < numPoints; i++)
		out << " "
				<< stroke[i].position.x << " "
				<< stroke[i].position.y << " "
				<< stroke[i].pressure << " "
				<< stroke[i].timestamp;

	out << std::endl;
}

