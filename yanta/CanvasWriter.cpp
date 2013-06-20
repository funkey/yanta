#include <fstream>
#include "CanvasWriter.h"

CanvasWriter::CanvasWriter(const std::string& filename) :
	_filename(filename) {

	registerInput(_canvas, "canvas");
}

void
CanvasWriter::write() {

	std::ofstream out(_filename.c_str());

	// write the file version (currently, we only have one)
	out << 1 << std::endl;

	writeStrokePoints(out, _canvas->getStrokePoints());

	// skip the last stroke, if it wasn't started, yet
	unsigned int numStrokes = _canvas->numStrokes();
	if (numStrokes > 0 && _canvas->currentStroke().size() <= 1)
		numStrokes--;

	out << numStrokes << std::endl;;

	for (unsigned int i = 0; i < numStrokes; i++)
		writeStroke(out, _canvas->getPage(0).getStroke(i));
}

void
CanvasWriter::writeStrokePoints(std::ofstream& out, const StrokePoints& points) {

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
CanvasWriter::writeStroke(std::ofstream& out, const Stroke& stroke) {

	out << " "
		<< stroke.begin() << " " << stroke.end() << " "
		<< stroke.getStyle().width() << " "
		<< stroke.getStyle().getRed() << " "
		<< stroke.getStyle().getGreen() << " "
		<< stroke.getStyle().getBlue() << " "
		<< stroke.getStyle().getAlpha() << " ";
}

