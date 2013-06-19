#ifndef YANTA_BACKEND_H__
#define YANTA_BACKEND_H__

#include <gui/PenSignals.h>
#include <pipeline/all.h>
#include "CanvasPainter.h"
#include "CanvasSignals.h"
#include "PenMode.h"

class Backend : public pipeline::SimpleProcessNode<> {

public:

	Backend();

private:

	void updateOutputs();

	void onModified(const pipeline::Modified&);

	void onPenDown(const gui::PenDown& signal);
	void onPenUp(const gui::PenUp& signal);
	void onPenMove(const gui::PenMove& signal);

	pipeline::Output<Canvas> _canvas;
	pipeline::Input<PenMode>  _penMode;
	pipeline::Input<Canvas>  _initialCanvas;

	bool _penDown;

	bool _erase;
	util::point<Canvas::Precision> _previousErasePosition;

	bool _initialCanvasModified;

	signals::Slot<ChangedArea> _changedArea;
};

#endif // YANTA_BACKEND_H__

