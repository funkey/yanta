#include <gui/Modifiers.h>
#include <util/Logger.h>
#include "Backend.h"

logger::LogChannel backendlog("backendlog", "[Backend] ");

Backend::Backend() :
	_penDown(false),
	_erase(false),
	_initialCanvasModified(false) {

	registerOutput(_canvas, "canvas");
	registerInput(_penMode, "pen mode");
	registerInput(_initialCanvas, "initial canvas", pipeline::Optional);

	_canvas.registerForwardSlot(_changedArea);
	_canvas.registerForwardCallback(&Backend::onPenDown, this);
	_canvas.registerForwardCallback(&Backend::onPenMove, this);
	_canvas.registerForwardCallback(&Backend::onPenUp, this);

	_initialCanvas.registerBackwardCallback(&Backend::onModified, this);
}

void
Backend::updateOutputs() {

	if (_initialCanvas && _initialCanvasModified) {

		LOG_DEBUG(backendlog) << "have initial canvas, loading them" << std::endl;
		*_canvas = *_initialCanvas;

		_initialCanvasModified = false;
	}
}

void
Backend::onModified(const pipeline::Modified&) {

	_initialCanvasModified = true;
}

void
Backend::onPenDown(const gui::PenDown& signal) {

	LOG_DEBUG(backendlog) << "pen down (button " << signal.button << ")" << std::endl;

	if (signal.button == gui::buttons::Left) {

		_penDown = true;

		if (!_erase) {

			LOG_DEBUG(backendlog) << "accepting" << std::endl;

			_canvas->createNewStroke();
			_canvas->currentStroke().setStyle(_penMode->getStyle());
			_canvas->addStrokePoint(StrokePoint(signal.position, signal.pressure, signal.timestamp));
		}
	}

	if (signal.button == gui::buttons::Middle) {

		_erase = true;
		_previousErasePosition = signal.position;

		if (_penDown) {

			_canvas->addStrokePoint(StrokePoint(signal.position, signal.pressure, signal.timestamp));
			_canvas->finishCurrentStroke();

			setDirty(_canvas);
		}
	}
}

void
Backend::onPenUp(const gui::PenUp& signal) {

	LOG_DEBUG(backendlog) << "pen up (button " << signal.button << ")" << std::endl;

	if (signal.button == gui::buttons::Left) {

		_penDown = false;

		if (!_erase) {

			LOG_DEBUG(backendlog) << "accepting" << std::endl;

			_canvas->addStrokePoint(StrokePoint(signal.position, signal.pressure, signal.timestamp));
			_canvas->finishCurrentStroke();

			setDirty(_canvas);
		}
	}

	if (signal.button == gui::buttons::Middle) {

		_erase = false;

		if (_penDown) {

			_canvas->createNewStroke();
			_canvas->currentStroke().setStyle(_penMode->getStyle());
			_canvas->addStrokePoint(StrokePoint(signal.position, signal.pressure, signal.timestamp));
		}
	}
}

void
Backend::onPenMove(const gui::PenMove& signal) {

	if (!_penDown)
		return;

	LOG_ALL(backendlog) << "pen move with modifiers " << signal.modifiers << std::endl;

	if (_erase) {

		util::rect<Canvas::Precision> dirtyArea = _canvas->erase(_previousErasePosition, signal.position);

		_previousErasePosition = signal.position;

		if (!dirtyArea.isZero()) {

			ChangedArea signal(dirtyArea);
			_changedArea(signal);
		}

	} else {

		_canvas->addStrokePoint(StrokePoint(signal.position, signal.pressure, signal.timestamp));
		setDirty(_canvas);
	}
}
