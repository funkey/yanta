#ifndef CANVAS_H__
#define CANVAS_H__

#include <gui/PenSignals.h>
#include <gui/KeySignals.h>
#include <pipeline/all.h>
#include "CanvasPainter.h"

class Canvas : public pipeline::SimpleProcessNode<> {

public:

	Canvas();

private:

	void updateOutputs();

	void onKeyDown(const gui::KeyDown& signal);

	void onPenDown(const gui::PenDown& signal);
	void onPenUp(const gui::PenUp& signal);
	void onPenMove(const gui::PenMove& signal);

	pipeline::Output<CanvasPainter> _painter;

	signals::Slot<const gui::ContentChanged> _contentChanged;

	boost::shared_ptr<Strokes> _strokes;

	bool _penDown;
};

#endif // CANVAS_H__

