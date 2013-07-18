#ifndef YANTA_CANVAS_READER_H__
#define YANTA_CANVAS_READER_H__

#include <string>

#include <pipeline/all.h>
#include "Canvas.h"

class CanvasReader : public pipeline::SimpleProcessNode<> {

public:

	CanvasReader(const std::string& filename);

private:

	void updateOutputs();

	void readStrokePoints(std::ifstream& in);

	void readPage(std::ifstream& in, unsigned int page, unsigned int fileVersion);

	void readStroke(std::ifstream& in, unsigned int page, unsigned int fileVersion);

	pipeline::Output<Canvas> _canvas;

	std::string _filename;
};

#endif // YANTA_CANVAS_READER_H__

