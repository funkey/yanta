#include <gui/Modifiers.h>
#include <util/Logger.h>
#include "Backend.h"

logger::LogChannel backendlog("backendlog", "[Backend] ");

Backend::Backend() :
	_penDown(false),
	_mode(Draw),
	_initialDocumentModified(false) {

	registerInput(_initialDocument, "initial document", pipeline::Optional);
	registerInput(_penMode, "pen mode");
	registerInput(_osdRequest, "osd request", pipeline::Optional);
	registerOutput(_document, "document");
	registerOutput(_overlay, "overlay");

	_initialDocument.registerBackwardCallback(&Backend::onModified, this);
	_osdRequest.registerBackwardCallback(&Backend::onAddPage, this);

	_document.registerForwardSlot(_documentChangedArea);
	_document.registerForwardSlot(_strokePointAdded);
	_document.registerForwardCallback(&Backend::onPenDown, this);
	_document.registerForwardCallback(&Backend::onPenMove, this);
	_document.registerForwardCallback(&Backend::onPenUp, this);

	_overlay.registerForwardSlot(_overlayChangedArea);
	_overlay.registerForwardSlot(_lassoPointAdded);
}

void
Backend::cleanup() {

	anchorSelection();
}

void
Backend::updateOutputs() {

	if (!_initialDocumentModified)
		return;

	if (_initialDocument && _initialDocument->numPages() > 0) {

		LOG_DEBUG(backendlog) << "have initial document, loading them" << std::endl;
		LOG_ALL(backendlog) << "initial document has " << _initialDocument->numStrokes() << " strokes on " << _initialDocument->numPages() << " pages" << std::endl;

		*_document = *_initialDocument;

		LOG_ALL(backendlog) << "copy has " << _document->numStrokes() << " strokes on " << _document->numPages() << " pages" << std::endl;

	} else {

		LOG_DEBUG(backendlog) << "create new document with two pages" << std::endl;

		_document->createPage(
				util::point<DocumentPrecision>(0.0, 0.0),
				util::point<PagePrecision>(210.0, 297.0) /* DIN A4 */);
	}

	_initialDocumentModified = false;
}

void
Backend::onModified(const pipeline::Modified&) {

	_initialDocumentModified = true;
}

void
Backend::onPenDown(const gui::PenDown& signal) {

	LOG_DEBUG(backendlog) << "pen down (button " << signal.button << ")" << std::endl;

	if (signal.button == gui::buttons::Left) {

		_penDown = true;

		if (_selection && _selection->getBoundingBox().contains(signal.position)) {

			_mode = DragSelection;
			_previousPosition = signal.position;
			return;
		}

		if (_penMode->getMode() == PenMode::Lasso) {

			LOG_ALL(backendlog) << "starting a new lasso" << std::endl;

			_lasso = boost::make_shared<Lasso>();
			_lasso->addPoint(signal.position);
			_overlay->add(_lasso);
			_lassoPointAdded();

			return;
		}

		if (_mode == Draw) {

			LOG_DEBUG(backendlog) << "accepting" << std::endl;

			_document->createNewStroke(signal.position, signal.pressure, signal.timestamp);
			_document->setCurrentStrokeStyle(_penMode->getStyle());
		}
	}

	if (signal.button == gui::buttons::Middle) {

		_mode = Erase;
		_previousPosition = signal.position;

		if (_penDown) {

			_document->addStrokePoint(signal.position, signal.pressure, signal.timestamp);
			_document->finishCurrentStroke();

			_strokePointAdded();
		}
	}
}

void
Backend::onPenUp(const gui::PenUp& signal) {

	LOG_DEBUG(backendlog) << "pen up (button " << signal.button << ")" << std::endl;

	if (signal.button == gui::buttons::Left) {

		_penDown = false;

		if (_mode == DragSelection) {

			_mode = Draw;
			return;
		}

		if (_penMode->getMode() == PenMode::Lasso) {

			anchorSelection();

			_selection = boost::make_shared<Selection>(Selection::CreateFromPath(_lasso->getPath(), *_document));

			_overlay->remove(_lasso);
			_overlay->add(_selection);

			OverlayChangedArea overlaySignal(_lasso->getBoundingBox());
			_overlayChangedArea(overlaySignal);

			DocumentChangedArea documentSignal(_selection->getBoundingBox());
			_documentChangedArea(documentSignal);

			_lasso.reset();

			return;
		}

		if (_mode == Draw) {

			LOG_DEBUG(backendlog) << "accepting" << std::endl;

			_document->addStrokePoint(signal.position, signal.pressure, signal.timestamp);
			_document->finishCurrentStroke();

			_strokePointAdded();
		}
	}

	if (signal.button == gui::buttons::Middle) {

		_mode = Draw;

		if (_penDown) {

			_document->createNewStroke(signal.position, signal.pressure, signal.timestamp);
			_document->setCurrentStrokeStyle(_penMode->getStyle());
		}
	}
}

void
Backend::onPenMove(const gui::PenMove& signal) {

	if (!_penDown)
		return;

	LOG_ALL(backendlog) << "pen move with modifiers " << signal.modifiers << std::endl;

	if (_mode == DragSelection) {

		util::rect<DocumentPrecision> changed = _selection->getBoundingBox();

		_selection->shift(signal.position - _previousPosition);
		_previousPosition = signal.position;

		changed.fit(_selection->getBoundingBox());

		OverlayChangedArea signal(changed);
		_overlayChangedArea(signal);

		return;
	}

	if (_penMode->getMode() == PenMode::Lasso) {

		_lasso->addPoint(signal.position);
		_lassoPointAdded();

		return;
	}

	if (_mode == Erase) {

		util::rect<DocumentPrecision> dirtyArea = _document->erase(_previousPosition, signal.position);

		_previousPosition = signal.position;

		if (!dirtyArea.isZero()) {

			DocumentChangedArea signal(dirtyArea);
			_documentChangedArea(signal);
		}

	} else {

		_document->addStrokePoint(signal.position, signal.pressure, signal.timestamp);
		_strokePointAdded();
	}
}

void
Backend::onAddPage(const AddPage& /*signal*/) {

	// the number of the new page to add
	int pageNum = _document->numPages();

	// let the new page be of the size of the most recent page
	const util::point<PagePrecision>& size = _document->getPage(pageNum - 1).getSize();

	int pageX = pageNum%2;
	int pageY = pageNum/2;

	// simple 2-pages layout
	// TODO: get layout from current OsdRequest input
	util::point<DocumentPrecision> position(pageX*(size.x + size.x/10), pageY*(size.y + size.y/8));

	_document->createPage(position, size);

	// inform about dirty area
	DocumentChangedArea signal(util::rect<DocumentPrecision>(position.x, position.y, position.x + size.x, position.y + size.y));
	_documentChangedArea(signal);
}

void
Backend::anchorSelection() {

	if (_selection) {

		_selection->anchor(*_document);
		_overlay->remove(_selection);

		DocumentChangedArea documentSignal(_selection->getBoundingBox());
		_documentChangedArea(documentSignal);
	}
}
