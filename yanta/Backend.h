#ifndef YANTA_BACKEND_H__
#define YANTA_BACKEND_H__

#include <gui/PenSignals.h>
#include <pipeline/all.h>
#include "CanvasPainter.h"
#include "CanvasSignals.h"
#include "PenMode.h"
#include "OsdRequest.h"
#include "OsdSignals.h"

class Backend : public pipeline::SimpleProcessNode<> {

public:

	Backend();

private:

	void updateOutputs();

	void onModified(const pipeline::Modified&);

	void onPenDown(const gui::PenDown& signal);
	void onPenUp(const gui::PenUp& signal);
	void onPenMove(const gui::PenMove& signal);

	void onAddPage(const AddPage& signal);

	pipeline::Input<Canvas>     _initialCanvas;
	pipeline::Input<PenMode>    _penMode;
	pipeline::Input<OsdRequest> _osdRequest;
	pipeline::Output<Canvas>    _canvas;

	bool _penDown;

	bool _erase;
	util::point<CanvasPrecision> _previousErasePosition;

	bool _initialCanvasModified;

	signals::Slot<ChangedArea>      _changedArea;
	signals::Slot<StrokePointAdded> _strokePointAdded;
};

#endif // YANTA_BACKEND_H__

