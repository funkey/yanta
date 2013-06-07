#include <gui/Modifiers.h>
#include <util/Logger.h>
#include "Canvas.h"

logger::LogChannel canvaslog("canvaslog", "[Canvas] ");

Canvas::Canvas() :
	_penDown(false) {

	registerOutput(_strokes, "strokes");

	_strokes.registerForwardCallback(&Canvas::onPenDown, this);
	_strokes.registerForwardCallback(&Canvas::onPenMove, this);
	_strokes.registerForwardCallback(&Canvas::onPenUp, this);
}

void
Canvas::onPenDown(const gui::PenDown& signal) {

	LOG_DEBUG(canvaslog) << "pen down (button " << signal.button << ")" << std::endl;

	if (signal.button != gui::buttons::Left)
		return;

	if (signal.modifiers & gui::keys::ControlDown)
		return;

	LOG_DEBUG(canvaslog) << "accepting" << std::endl;

	_strokes->createNewStroke();
	_strokes->currentStroke().push_back(StrokePoint(signal.position, signal.pressure, signal.timestamp));
	_penDown = true;

	setDirty(_strokes);
}

void
Canvas::onPenUp(const gui::PenUp& signal) {

	LOG_DEBUG(canvaslog) << "pen up (button " << signal.button << ")" << std::endl;

	if (signal.button != gui::buttons::Left)
		return;

	if (signal.modifiers & gui::keys::ControlDown)
		return;

	LOG_DEBUG(canvaslog) << "accepting" << std::endl;

	_strokes->currentStroke().push_back(StrokePoint(signal.position, signal.pressure, signal.timestamp));
	_strokes->finishCurrentStroke();
	_penDown = false;

	setDirty(_strokes);
}

void
Canvas::onPenMove(const gui::PenMove& signal) {

	if (!_penDown)
		return;

	LOG_DEBUG(canvaslog) << "pen move" << std::endl;

	_strokes->currentStroke().push_back(StrokePoint(signal.position, signal.pressure, signal.timestamp));

	setDirty(_strokes);
}
