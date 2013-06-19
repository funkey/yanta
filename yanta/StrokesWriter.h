#ifndef YANTA_STROKES_WRITER_H__
#define YANTA_STROKES_WRITER_H__

#include <string>

#include <pipeline/all.h>
#include "Strokes.h"

class StrokesWriter : public pipeline::SimpleProcessNode<> {

public:

	StrokesWriter(const std::string& filename);

	void write();

private:

	void updateOutputs() {}

	void writeStrokePoints(std::ofstream& out, const StrokePoints& points);

	void writeStroke(std::ofstream& out, const Stroke& stroke);

	pipeline::Input<Strokes> _strokes;

	std::string _filename;
};

#endif // YANTA_STROKES_WRITER_H__

