#ifndef YANTA_CANVAS_WRITER_H__
#define YANTA_CANVAS_WRITER_H__

#include <string>

#include <pipeline/all.h>

#include <document/Document.h>

class DocumentWriter : public pipeline::SimpleProcessNode<> {

public:

	DocumentWriter(const std::string& filename);

	void write();

private:

	void updateOutputs() {}

	void writeStrokePoints(std::ofstream& out, const StrokePoints& points);

	void writePage(std::ofstream& out, const Page& page);

	void writeStroke(std::ofstream& out, const Stroke& stroke);

	pipeline::Input<Document> _document;

	std::string _filename;
};

#endif // YANTA_CANVAS_WRITER_H__

