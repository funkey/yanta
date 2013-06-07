#include <util/Logger.h>
#include "CanvasView.h"

logger::LogChannel canvasviewlog("canvasviewlog", "[CanvasView] ");

CanvasView::CanvasView() {

	registerInput(_strokes, "strokes");
	registerOutput(_painter, "painter");

	_painter.registerForwardSlot(_contentChanged);

	// establish pointer signal filter
	PointerSignalFilter::filterBackward(_painter, _strokes, this);
}

void
CanvasView::updateOutputs() {

	_painter->setStrokes(_strokes);

	// TODO: do we need that?
	_contentChanged();
}
