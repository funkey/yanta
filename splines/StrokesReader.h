#ifndef SPLINES_STROKES_READER_H__
#define SPLINES_STROKES_READER_H__

#include <string>

#include <pipeline/all.h>
#include "Strokes.h"

class StrokesReader : public pipeline::SimpleProcessNode<> {

public:

	StrokesReader(const std::string& filename);

private:

	void updateOutputs();

	void readStroke(std::ifstream& in);

	pipeline::Output<Strokes> _strokes;

	std::string _filename;
};

#endif // SPLINES_STROKES_READER_H__

