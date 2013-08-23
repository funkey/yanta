#include <fstream>

#include <util/ProgramOptions.h>
#include <util/Logger.h>
#include "DocumentWriter.h"

logger::LogChannel documentwriterlog("documentwriterlog", "[DocumentWriter] ");

util::ProgramOption optionAutosaveInterval(
		util::_long_name        = "autosaveInterval",
		util::_short_name       = "a",
		util::_description_text = "The number of seconds between two auto-saves.",
		util::_default_value    = 60);

DocumentWriter::DocumentWriter(const std::string& filename) :
	_filename(filename),
	_autosaveThread(boost::bind(&DocumentWriter::autosave, this, optionAutosaveInterval.as<unsigned int>())) {

	registerInput(_document, "document");
}

DocumentWriter::~DocumentWriter() {

	_autosaveThread.interrupt();
	_autosaveThread.join();
}

void
DocumentWriter::write(const std::string& filename) {

	updateInputs();

	LOG_DEBUG(documentwriterlog) << "saving to " << (filename == "" ? _filename : filename) << std::endl;

	std::ofstream out(filename == "" ? _filename.c_str() : filename.c_str());

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

void
DocumentWriter::autosave(unsigned int interval) {

	LOG_DEBUG(documentwriterlog) << "starting autosave thread" << std::endl;

	try {

		while (true) {

			boost::this_thread::sleep(boost::posix_time::time_duration(0, 0, interval));

			write(std::string(".") + _filename + ".swp");
		}
	} catch (boost::thread_interrupted& e) {}
}
