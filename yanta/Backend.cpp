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

	if (!_initialCanvasModified)
		return;

	if (_initialCanvas && _initialCanvas->numPages() > 0) {

		LOG_DEBUG(backendlog) << "have initial canvas, loading them" << std::endl;
		LOG_ALL(backendlog) << "initial canvas has " << _initialCanvas->numStrokes() << " strokes on " << _initialCanvas->numPages() << " pages" << std::endl;

		*_canvas = *_initialCanvas;

		LOG_ALL(backendlog) << "copy has " << _canvas->numStrokes() << " strokes on " << _canvas->numPages() << " pages" << std::endl;

	} else {

		LOG_DEBUG(backendlog) << "create new canvas with two pages" << std::endl;

		_canvas->createPage(
				util::point<CanvasPrecision>(0.0, 0.0),
				util::point<PagePrecision>(210.0, 297.0) /* DIN A4 */);

		_canvas->createPage(
				util::point<CanvasPrecision>(300.0, 0.0),
				util::point<PagePrecision>(210.0, 297.0) /* DIN A4 */);
	}

	_initialCanvasModified = false;
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

			_canvas->createNewStroke(signal.position, signal.pressure, signal.timestamp);
			_canvas->currentStroke().setStyle(_penMode->getStyle());
		}
	}

	if (signal.button == gui::buttons::Middle) {

		_erase = true;
		_previousErasePosition = signal.position;

		if (_penDown) {

			_canvas->addStrokePoint(signal.position, signal.pressure, signal.timestamp);
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

			_canvas->addStrokePoint(signal.position, signal.pressure, signal.timestamp);
			_canvas->finishCurrentStroke();

			setDirty(_canvas);
		}
	}

	if (signal.button == gui::buttons::Middle) {

		_erase = false;

		if (_penDown) {

			_canvas->createNewStroke(signal.position, signal.pressure, signal.timestamp);
			_canvas->currentStroke().setStyle(_penMode->getStyle());
		}
	}
}

void
Backend::onPenMove(const gui::PenMove& signal) {

	if (!_penDown)
		return;

	LOG_ALL(backendlog) << "pen move with modifiers " << signal.modifiers << std::endl;

	if (_erase) {

		util::rect<CanvasPrecision> dirtyArea = _canvas->erase(_previousErasePosition, signal.position);

		_previousErasePosition = signal.position;

		if (!dirtyArea.isZero()) {

			ChangedArea signal(dirtyArea);
			_changedArea(signal);
		}

	} else {

		_canvas->addStrokePoint(signal.position, signal.pressure, signal.timestamp);
		setDirty(_canvas);
	}
}
