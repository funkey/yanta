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
	out << 1 << std::endl;

	writeStrokePoints(out, _strokes->getStrokePoints());

	// skip the last stroke, if it wasn't started, yet
	unsigned int numStrokes = _strokes->numStrokes();
	if (numStrokes > 0 && _strokes->currentStroke().size() <= 1)
		numStrokes--;

	out << numStrokes << std::endl;;

	for (unsigned int i = 0; i < numStrokes; i++)
		writeStroke(out, _strokes->getStroke(i));
}

void
StrokesWriter::writeStrokePoints(std::ofstream& out, const StrokePoints& points) {

	unsigned long numPoints = points.size();
	out << numPoints;

	for (unsigned long i = 0; i < numPoints; i++)
		out << " "
				<< points[i].position.x << " "
				<< points[i].position.y << " "
				<< points[i].pressure << " "
				<< points[i].timestamp;

	out << std::endl;
}

void
StrokesWriter::writeStroke(std::ofstream& out, const Stroke& stroke) {

	out << " "
		<< stroke.begin() << " " << stroke.end() << " "
		<< stroke.getStyle().width() << " "
		<< stroke.getStyle().getRed() << " "
		<< stroke.getStyle().getGreen() << " "
		<< stroke.getStyle().getBlue() << " "
		<< stroke.getStyle().getAlpha() << " ";
}

