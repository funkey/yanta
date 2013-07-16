#include <gui/Modifiers.h>
#include <util/Logger.h>
#include "Backend.h"

logger::LogChannel backendlog("backendlog", "[Backend] ");

Backend::Backend() :
	_penDown(false),
	_erase(false),
	_initialCanvasModified(false) {

	registerInput(_initialCanvas, "initial canvas", pipeline::Optional);
	registerInput(_penMode, "pen mode");
	registerInput(_osdRequest, "osd request", pipeline::Optional);
	registerOutput(_canvas, "canvas");
	registerOutput(_overlay, "overlay");

	_initialCanvas.registerBackwardCallback(&Backend::onModified, this);
	_osdRequest.registerBackwardCallback(&Backend::onAddPage, this);

	_canvas.registerForwardSlot(_canvasChangedArea);
	_canvas.registerForwardSlot(_strokePointAdded);
	_canvas.registerForwardCallback(&Backend::onPenDown, this);
	_canvas.registerForwardCallback(&Backend::onPenMove, this);
	_canvas.registerForwardCallback(&Backend::onPenUp, this);

	_overlay.registerForwardSlot(_overlayChangedArea);
	_overlay.registerForwardSlot(_lassoPointAdded);
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

		if (_penMode->getMode() == PenMode::Lasso) {

			LOG_ALL(backendlog) << "starting a new lasso" << std::endl;

			_lasso = boost::make_shared<Lasso>();
			_lasso->addPoint(signal.position);
			_overlay->add(_lasso);
			_lassoPointAdded();

			return;
		}

		if (!_erase) {

			LOG_DEBUG(backendlog) << "accepting" << std::endl;

			_canvas->createNewStroke(signal.position, signal.pressure, signal.timestamp);
			_canvas->setCurrentStrokeStyle(_penMode->getStyle());
		}
	}

	if (signal.button == gui::buttons::Middle) {

		_erase = true;
		_previousErasePosition = signal.position;

		if (_penDown) {

			_canvas->addStrokePoint(signal.position, signal.pressure, signal.timestamp);
			_canvas->finishCurrentStroke();

			_strokePointAdded();
		}
	}
}

void
Backend::onPenUp(const gui::PenUp& signal) {

	LOG_DEBUG(backendlog) << "pen up (button " << signal.button << ")" << std::endl;

	if (signal.button == gui::buttons::Left) {

		_penDown = false;

		if (_penMode->getMode() == PenMode::Lasso) {

			_selection = boost::make_shared<Selection>(Selection::CreateFromPath(_lasso->getPath(), *_canvas));

			_overlay->remove(_lasso);
			_overlay->add(_selection);

			OverlayChangedArea overlaySignal(_lasso->getBoundingBox());
			_overlayChangedArea(overlaySignal);

			CanvasChangedArea canvasSignal(_selection->getBoundingBox());
			_canvasChangedArea(canvasSignal);

			_lasso.reset();

			return;
		}

		if (!_erase) {

			LOG_DEBUG(backendlog) << "accepting" << std::endl;

			_canvas->addStrokePoint(signal.position, signal.pressure, signal.timestamp);
			_canvas->finishCurrentStroke();

			_strokePointAdded();
		}
	}

	if (signal.button == gui::buttons::Middle) {

		_erase = false;

		if (_penDown) {

			_canvas->createNewStroke(signal.position, signal.pressure, signal.timestamp);
			_canvas->setCurrentStrokeStyle(_penMode->getStyle());
		}
	}
}

void
Backend::onPenMove(const gui::PenMove& signal) {

	if (!_penDown)
		return;

	LOG_ALL(backendlog) << "pen move with modifiers " << signal.modifiers << std::endl;

	if (_penMode->getMode() == PenMode::Lasso) {

		_lasso->addPoint(signal.position);
		_lassoPointAdded();

		return;
	}

	if (_erase) {

		util::rect<CanvasPrecision> dirtyArea = _canvas->erase(_previousErasePosition, signal.position);

		_previousErasePosition = signal.position;

		if (!dirtyArea.isZero()) {

			CanvasChangedArea signal(dirtyArea);
			_canvasChangedArea(signal);
		}

	} else {

		_canvas->addStrokePoint(signal.position, signal.pressure, signal.timestamp);
		_strokePointAdded();
	}
}

void
Backend::onAddPage(const AddPage& /*signal*/) {

	// the number of the new page to add
	int pageNum = _canvas->numPages();

	// let the new page be of the size of the most recent page
	const util::point<PagePrecision>& size = _canvas->getPage(pageNum - 1).getSize();

	int pageX = pageNum%2;
	int pageY = pageNum/2;

	// simple 2-pages layout
	// TODO: get layout from current OsdRequest input
	util::point<CanvasPrecision> position(pageX*(size.x + size.x/10), pageY*(size.y + size.y/8));

	_canvas->createPage(position, size);

	// inform about dirty area
	CanvasChangedArea signal(util::rect<CanvasPrecision>(position.x, position.y, position.x + size.x, position.y + size.y));
	_canvasChangedArea(signal);
}
