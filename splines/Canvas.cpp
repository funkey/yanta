#include <gui/Modifiers.h>
#include <util/Logger.h>
#include "Canvas.h"

logger::LogChannel canvaslog("canvaslog", "[Canvas] ");

Canvas::Canvas() :
	_penDown(false),
	_erase(false),
	_initialStrokesModified(false) {

	registerOutput(_strokes, "strokes");
	registerInput(_penMode, "pen mode");
	registerInput(_initialStrokes, "initial strokes", pipeline::Optional);

	_strokes.registerForwardSlot(_changedArea);
	_strokes.registerForwardCallback(&Canvas::onPenDown, this);
	_strokes.registerForwardCallback(&Canvas::onPenMove, this);
	_strokes.registerForwardCallback(&Canvas::onPenUp, this);

	_initialStrokes.registerBackwardCallback(&Canvas::onModified, this);
}

void
Canvas::updateOutputs() {

	if (_initialStrokes && _initialStrokesModified) {

		LOG_DEBUG(canvaslog) << "have initial strokes, loading them" << std::endl;
		*_strokes = *_initialStrokes;

		_initialStrokesModified = false;
	}
}

void
Canvas::onModified(const pipeline::Modified&) {

	_initialStrokesModified = true;
}

void
Canvas::onPenDown(const gui::PenDown& signal) {

	LOG_DEBUG(canvaslog) << "pen down (button " << signal.button << ")" << std::endl;

	if (signal.button == gui::buttons::Left) {

		_penDown = true;

		if (!_erase) {

			LOG_DEBUG(canvaslog) << "accepting" << std::endl;

			_strokes->createNewStroke();
			_strokes->currentStroke().setStyle(_penMode->getStyle());
			_strokes->addStrokePoint(StrokePoint(signal.position, signal.pressure, signal.timestamp));
		}
	}

	if (signal.button == gui::buttons::Middle) {

		_erase = true;
		_previousErasePosition = signal.position;

		if (_penDown) {

			_strokes->addStrokePoint(StrokePoint(signal.position, signal.pressure, signal.timestamp));
			_strokes->finishCurrentStroke();

			setDirty(_strokes);
		}
	}
}

void
Canvas::onPenUp(const gui::PenUp& signal) {

	LOG_DEBUG(canvaslog) << "pen up (button " << signal.button << ")" << std::endl;

	if (signal.button == gui::buttons::Left) {

		_penDown = false;

		if (!_erase) {

			LOG_DEBUG(canvaslog) << "accepting" << std::endl;

			_strokes->addStrokePoint(StrokePoint(signal.position, signal.pressure, signal.timestamp));
			_strokes->finishCurrentStroke();

			setDirty(_strokes);
		}
	}

	if (signal.button == gui::buttons::Middle) {

		_erase = false;

		if (_penDown) {

			_strokes->createNewStroke();
			_strokes->currentStroke().setStyle(_penMode->getStyle());
			_strokes->addStrokePoint(StrokePoint(signal.position, signal.pressure, signal.timestamp));
		}
	}
}

void
Canvas::onPenMove(const gui::PenMove& signal) {

	if (!_penDown)
		return;

	LOG_ALL(canvaslog) << "pen move with modifiers " << signal.modifiers << std::endl;

	if (_erase) {

		util::rect<double> dirtyArea = _strokes->erase(_previousErasePosition, signal.position);

		_previousErasePosition = signal.position;

		if (!dirtyArea.isZero()) {

			ChangedArea signal(dirtyArea);
			_changedArea(signal);
		}

	} else {

		_strokes->addStrokePoint(StrokePoint(signal.position, signal.pressure, signal.timestamp));
		setDirty(_strokes);
	}
}
