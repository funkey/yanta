#ifndef YANTA_CANVAS_PDF_WRITER_H__
#define YANTA_CANVAS_PDF_WRITER_H__

#include <string>

#include <pipeline/all.h>
#include "Canvas.h"

class CanvasPdfWriter : public pipeline::SimpleProcessNode<> {

public:

	CanvasPdfWriter(const std::string& filename);

	void write();

private:

	void updateOutputs() {}

	pipeline::Input<Canvas> _canvas;

	std::string _filename;
};

#endif // YANTA_CANVAS_PDF_WRITER_H__

