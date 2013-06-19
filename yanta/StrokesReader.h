#ifndef YANTA_STROKES_READER_H__
#define YANTA_STROKES_READER_H__

#include <string>

#include <pipeline/all.h>
#include "Strokes.h"

class StrokesReader : public pipeline::SimpleProcessNode<> {

public:

	StrokesReader(const std::string& filename);

private:

	void updateOutputs();

	void readStrokePoints(std::ifstream& in);

	void readStroke(std::ifstream& in);

	pipeline::Output<Strokes> _strokes;

	std::string _filename;
};

#endif // YANTA_STROKES_READER_H__
