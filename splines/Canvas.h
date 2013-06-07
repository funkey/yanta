#ifndef CANVAS_H__
#define CANVAS_H__

#include <gui/PenSignals.h>
#include <pipeline/all.h>
#include "CanvasPainter.h"

class Canvas : public pipeline::SimpleProcessNode<> {

public:

	Canvas();

private:

	void updateOutputs();

	void onModified(const pipeline::Modified&);

	void onPenDown(const gui::PenDown& signal);
	void onPenUp(const gui::PenUp& signal);
	void onPenMove(const gui::PenMove& signal);

	pipeline::Output<Strokes> _strokes;
	pipeline::Input<Strokes>  _initialStrokes;

	bool _penDown;

	bool _initialStrokesModified;
};

#endif // CANVAS_H__

