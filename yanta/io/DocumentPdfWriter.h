#ifndef YANTA_CANVAS_PDF_WRITER_H__
#define YANTA_CANVAS_PDF_WRITER_H__

#include <string>

#include <pipeline/all.h>

#include <document/Document.h>

class DocumentPdfWriter : public pipeline::SimpleProcessNode<> {

public:

	DocumentPdfWriter(const std::string& filename);

	void write();

private:

	void updateOutputs() {}

	pipeline::Input<Document> _document;

	std::string _filename;
};

#endif // YANTA_CANVAS_PDF_WRITER_H__

