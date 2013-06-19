#ifndef YANTA_CANVAS_WRITER_H__
#define YANTA_CANVAS_WRITER_H__

#include <string>

#include <pipeline/all.h>
#include "Canvas.h"

class CanvasWriter : public pipeline::SimpleProcessNode<> {

public:

	CanvasWriter(const std::string& filename);

	void write();

private:

	void updateOutputs() {}

	void writeStrokePoints(std::ofstream& out, const StrokePoints& points);

	void writeStroke(std::ofstream& out, const Stroke& stroke);

	pipeline::Input<Canvas> _canvas;

	std::string _filename;
};

#endif // YANTA_CANVAS_WRITER_H__

