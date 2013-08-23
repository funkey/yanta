#ifndef YANTA_CANVAS_READER_H__
#define YANTA_CANVAS_READER_H__

#include <string>

#include <pipeline/all.h>

#include <document/Document.h>

class DocumentReader : public pipeline::SimpleProcessNode<> {

public:

	DocumentReader(const std::string& filename);

private:

	void updateOutputs();

	void readStrokePoints(std::ifstream& in);

	void readPage(std::ifstream& in, unsigned int page, unsigned int fileVersion);

	void readStroke(std::ifstream& in, unsigned int page, unsigned int fileVersion);

	pipeline::Output<Document> _document;

	std::string _filename;
};

#endif // YANTA_CANVAS_READER_H__

