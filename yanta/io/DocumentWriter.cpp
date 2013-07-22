#include <fstream>

#include "DocumentWriter.h"

DocumentWriter::DocumentWriter(const std::string& filename) :
	_filename(filename) {

	registerInput(_document, "document");
}

void
DocumentWriter::write() {

	std::ofstream out(_filename.c_str());

	// write the file version
	out << 3 << std::endl;

	writeStrokePoints(out, _document->getStrokePoints());

	unsigned int numPages = _document->numPages();
	out << numPages << std::endl;

	for (unsigned int i = 0; i < numPages; i++)
		writePage(out, _document->getPage(i));
}

void
DocumentWriter::writeStrokePoints(std::ofstream& out, const StrokePoints& points) {

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
DocumentWriter::writePage(std::ofstream& out, const Page& page) {

	out << page.getShift().x << " " << page.getShift().y;
	out << " " << page.getSize().x << " " << page.getSize().y;

	unsigned int numStrokes = page.numStrokes();
	out << " " << numStrokes << std::endl;;

	for (unsigned int i = 0; i < numStrokes; i++)
		writeStroke(out, page.getStroke(i));

	out << std::endl;
}

void
DocumentWriter::writeStroke(std::ofstream& out, const Stroke& stroke) {

	out << " "
		<< stroke.begin() << " " << stroke.end() << " "
		<< stroke.getStyle().width() << " "
		<< stroke.getStyle().getRed() << " "
		<< stroke.getStyle().getGreen() << " "
		<< stroke.getStyle().getBlue() << " "
		<< stroke.getStyle().getAlpha() << " "
		<< stroke.getScale().x << " "
		<< stroke.getScale().y << " "
		<< stroke.getShift().x << " "
		<< stroke.getShift().y << " ";
}

