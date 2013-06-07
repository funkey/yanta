#ifndef SPLINES_CANVAS_VIEW_H__
#define SPLINES_CANVAS_VIEW_H__

#include <pipeline/all.h>
#include <gui/GuiSignals.h>
#include <gui/PointerSignalFilter.h>

#include "CanvasPainter.h"
#include "Strokes.h"

class CanvasView : public pipeline::SimpleProcessNode<>, public gui::PointerSignalFilter {

public:

	CanvasView();

private:

	/**
	 * Filter callback for generic 2D input signals. We just forward the signal, 
	 * there is nothing to do, here.
	 */
	bool filter(gui::PointerSignal&) { return true; }

	void updateOutputs();

	pipeline::Input<Strokes>        _strokes;
	pipeline::Output<CanvasPainter> _painter;

	// TODO: do we need that?
	signals::Slot<const gui::ContentChanged> _contentChanged;
};

#endif // SPLINES_CANVAS_VIEW_H__

