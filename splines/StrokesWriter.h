#ifndef SPLINES_STROKES_WRITER_H__
#define SPLINES_STROKES_WRITER_H__

#include <string>

#include <pipeline/all.h>
#include "Strokes.h"

class StrokesWriter : public pipeline::SimpleProcessNode<> {

public:

	StrokesWriter(const std::string& filename);

	void write();

private:

	void updateOutputs() {}

	void writeStroke(std::ofstream& out, const Stroke& stroke);

	pipeline::Input<Strokes> _strokes;

	std::string _filename;
};

#endif // SPLINES_STROKES_WRITER_H__

