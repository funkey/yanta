#include <gui/Modifiers.h>
#include <util/Logger.h>
#include "Canvas.h"

logger::LogChannel canvaslog("canvaslog", "[Canvas] ");

Canvas::Canvas() :
	_strokes(boost::make_shared<Strokes>()),
	_penDown(false) {

	registerOutput(_painter, "painter");

	_painter.registerForwardSlot(_contentChanged);
	_painter.registerForwardCallback(&Canvas::onPenDown, this);
	_painter.registerForwardCallback(&Canvas::onPenMove, this);
	_painter.registerForwardCallback(&Canvas::onPenUp, this);
}

void
Canvas::updateOutputs() {

	_painter->setStrokes(_strokes);
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
	_strokes->currentStroke().push_back(signal.position);
	_penDown = true;

	_contentChanged();
}

void
Canvas::onPenUp(const gui::PenUp& signal) {

	LOG_DEBUG(canvaslog) << "pen up (button " << signal.button << ")" << std::endl;

	if (signal.button != gui::buttons::Left)
		return;

	if (signal.modifiers & gui::keys::ControlDown)
		return;

	LOG_DEBUG(canvaslog) << "accepting" << std::endl;

	_strokes->currentStroke().push_back(signal.position);
	_strokes->finishCurrentStroke();
	_penDown = false;

	_contentChanged();
}

void
Canvas::onPenMove(const gui::PenMove& signal) {

	if (!_penDown)
		return;

	LOG_DEBUG(canvaslog) << "pen move" << std::endl;

	_strokes->currentStroke().push_back(signal.position);

	_contentChanged();
}
