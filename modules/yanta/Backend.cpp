#include <gui/Modifiers.h>
#include <util/Logger.h>
#include <tools/Erasor.h>
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
	registerOutput(_tools, "tools");

	_initialDocument.registerBackwardCallback(&Backend::onModified, this);
	_osdRequest.registerBackwardCallback(&Backend::onAddPage, this);

	_document.registerForwardSlot(_documentChangedArea);
	_document.registerForwardSlot(_strokePointAdded);
	_document.registerForwardCallback(&Backend::onPenDown, this);
	_document.registerForwardCallback(&Backend::onPenMove, this);
	_document.registerForwardCallback(&Backend::onPenUp, this);

	_tools.registerForwardSlot(_toolsChangedArea);
	_tools.registerForwardSlot(_lassoPointAdded);
	_tools.registerForwardSlot(_selectionMoved);
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

	_previousPosition = signal.position;

	if (signal.button == gui::buttons::Left) {

		_penDown = true;

		for (unsigned int i = 0; i < _document->size<Selection>(); i++) {

			if (_document->get<Selection>(i).getBoundingBox().contains(signal.position)) {

				_mode = DragSelection;
				_currentElement = i;

				return;
			}
		}

		if (_penMode->getMode() == PenMode::Lasso) {

			LOG_ALL(backendlog) << "starting a new lasso" << std::endl;

			_lasso = boost::make_shared<Lasso>();
			_lasso->addPoint(signal.position);
			_tools->add(_lasso);

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

		if (_penDown) {

			_document->addStrokePoint(signal.position, signal.pressure, signal.timestamp);
			_document->finishCurrentStroke();

			double penWidth = _penMode->getStyle().width();
			util::rect<DocumentPrecision> area(
					signal.position.x - penWidth,
					signal.position.y - penWidth,
					signal.position.x + penWidth,
					signal.position.y + penWidth);
			StrokePointAdded signal(area);
			_strokePointAdded(signal);
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

		} else if (_penMode->getMode() == PenMode::Lasso) {

			anchorSelection();

			Selection selection = Selection::CreateFromPath(_lasso->getPath(), *_document);

			if (selection.size() > 0) {

				_document->add(selection);

				ChangedArea documentSignal(selection.getBoundingBox());
				_documentChangedArea(documentSignal);
			}

			_tools->remove(_lasso);

			ChangedArea toolsSignal(_lasso->getBoundingBox());
			_toolsChangedArea(toolsSignal);

			_lasso.reset();

		} else if (_mode == Draw) {

			LOG_DEBUG(backendlog) << "accepting" << std::endl;

			_document->addStrokePoint(signal.position, signal.pressure, signal.timestamp);
			_document->finishCurrentStroke();

			double penWidth = _penMode->getStyle().width();
			util::rect<DocumentPrecision> area(_previousPosition.x, _previousPosition.y, _previousPosition.x, _previousPosition.y);
			area.fit(signal.position);
			area.minX -= penWidth;
			area.minY -= penWidth;
			area.maxX += penWidth;
			area.maxY += penWidth;
			StrokePointAdded signal(area);
			_strokePointAdded(signal);
		}

	} else if (signal.button == gui::buttons::Middle) {

		_mode = Draw;

		if (_penDown) {

			_document->createNewStroke(signal.position, signal.pressure, signal.timestamp);
			_document->setCurrentStrokeStyle(_penMode->getStyle());
		}
	}

	_previousPosition = signal.position;
}

void
Backend::onPenMove(const gui::PenMove& signal) {

	if (!_penDown)
		return;

	LOG_ALL(backendlog) << "pen move with modifiers " << signal.modifiers << std::endl;

	if (_mode == DragSelection) {

		Selection& selection = _document->get<Selection>(_currentElement);

		util::point<DocumentPrecision> shift = signal.position - _previousPosition;

		util::rect<DocumentPrecision> changed = selection.getBoundingBox();
		selection.shift(shift);
		changed.fit(selection.getBoundingBox());

		SelectionMoved movedSignal(changed, _currentElement, shift);
		_selectionMoved(movedSignal);

	} else if (_penMode->getMode() == PenMode::Lasso) {

		_lasso->addPoint(signal.position);
		LassoPointAdded pointAdded(_previousPosition, signal.position);
		_lassoPointAdded(pointAdded);

		// we don't want _previousPosition to be set to the current position
		return;

	} else if (_mode == Erase) {

		Erasor erasor(*_document);
		erasor.setMode(_penMode->getErasorMode());
		erasor.setRadius(2*_penMode->getStyle().width());
		util::rect<DocumentPrecision> dirtyArea = erasor.erase(_previousPosition, signal.position);

		if (!dirtyArea.isZero()) {

			ChangedArea changedSignal(dirtyArea);
			_documentChangedArea(changedSignal);
		}

	} else {

		_document->addStrokePoint(signal.position, signal.pressure, signal.timestamp);

		double penWidth = _penMode->getStyle().width();
		util::rect<DocumentPrecision> area(_previousPosition.x, _previousPosition.y, _previousPosition.x, _previousPosition.y);
		area.fit(signal.position);
		area.minX -= penWidth;
		area.minY -= penWidth;
		area.maxX += penWidth;
		area.maxY += penWidth;
		StrokePointAdded addedSignal(area);
		_strokePointAdded(addedSignal);
	}

	_previousPosition = signal.position;
}

void
Backend::onAddPage(const AddPage& /*signal*/) {

	// the number of the new page to add
	int pageNum = _document->numPages();

	// let the new page be of the size of the most recent page
	util::point<PagePrecision> size = _document->getPage(pageNum - 1).getSize();

	int pageX = pageNum%2;
	int pageY = pageNum/2;

	// simple 2-pages layout
	// TODO: get layout from current OsdRequest input
	util::point<DocumentPrecision> position(pageX*(size.x + size.x/10), pageY*(size.y + size.y/8));

	_document->createPage(position, size);

	// inform about dirty area
	ChangedArea signal(util::rect<DocumentPrecision>(position.x, position.y, position.x + size.x, position.y + size.y));
	_documentChangedArea(signal);
}

void
Backend::anchorSelection() {

	for (unsigned int i = 0; i < _document->size<Selection>(); i++) {

		Selection& selection = _document->get<Selection>(i);

		selection.anchor(*_document);

		ChangedArea changedArea(selection.getBoundingBox());
		_documentChangedArea(changedArea);
		_toolsChangedArea(changedArea);
	}

	_document->clear<Selection>();
}
