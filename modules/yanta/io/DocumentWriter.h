#ifndef YANTA_CANVAS_WRITER_H__
#define YANTA_CANVAS_WRITER_H__

#include <string>

#include <boost/thread.hpp>

#include <pipeline/all.h>

#include <document/Document.h>

class DocumentWriter : public pipeline::SimpleProcessNode<> {

public:

	DocumentWriter(const std::string& filename);

	~DocumentWriter();

	void write(const std::string& filename = "");

private:

	void updateOutputs() {}

	void writeStrokePoints(std::ofstream& out, const StrokePoints& points);

	void writePage(std::ofstream& out, const Page& page);

	void writeStroke(std::ofstream& out, const Stroke& stroke);

	/**
	 * Entry point for the auto-save thread.
	 */
	void autosave(unsigned int interval);

	pipeline::Input<Document> _document;

	std::string _filename;

	boost::thread _autosaveThread;
};

#endif // YANTA_CANVAS_WRITER_H__

