#ifndef YANTA_BACKEND_H__
#define YANTA_BACKEND_H__

#include <gui/PenSignals.h>
#include <pipeline/all.h>
#include "BackendPainter.h"
#include "CanvasSignals.h"
#include "PenMode.h"
#include "Lasso.h"
#include "OsdRequest.h"
#include "OsdSignals.h"
#include "Overlay.h"
#include "OverlaySignals.h"
#include "Selection.h"

class Backend : public pipeline::SimpleProcessNode<> {

public:

	Backend();

private:

	enum Mode {

		Draw,
		Erase,
		DragSelection
	};

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
	pipeline::Output<Overlay>   _overlay;

	bool _penDown;

	Mode _mode;
	util::point<CanvasPrecision> _previousPosition;

	bool _initialCanvasModified;

	boost::shared_ptr<Lasso>     _lasso;
	boost::shared_ptr<Selection> _selection;

	signals::Slot<CanvasChangedArea>  _canvasChangedArea;
	signals::Slot<StrokePointAdded>   _strokePointAdded;
	signals::Slot<OverlayChangedArea> _overlayChangedArea;
	signals::Slot<LassoPointAdded>    _lassoPointAdded;
};

#endif // YANTA_BACKEND_H__

