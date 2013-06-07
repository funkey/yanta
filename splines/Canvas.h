#ifndef CANVAS_H__
#define CANVAS_H__

#include <gui/PenSignals.h>
#include <pipeline/all.h>
#include "CanvasPainter.h"

class Canvas : public pipeline::SimpleProcessNode<> {

public:

	Canvas();

private:

	void updateOutputs() {}

	void onPenDown(const gui::PenDown& signal);
	void onPenUp(const gui::PenUp& signal);
	void onPenMove(const gui::PenMove& signal);

	pipeline::Output<Strokes> _strokes;

	bool _penDown;
};

#endif // CANVAS_H__

