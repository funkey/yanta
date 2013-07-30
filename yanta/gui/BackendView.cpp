#include <boost/timer/timer.hpp>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include "BackendView.h"

util::ProgramOption optionPenOffsetX(
	util::_long_name        = "penOffsetX",
	util::_description_text = "The offset in pixels for the pen tip. Set this value to avoid hiding the virtual pen tip under the pyhsical pen.",
	util::_default_value    = 0);

util::ProgramOption optionPenOffsetY(
	util::_long_name        = "penOffsetY",
	util::_description_text = "The offset in pixels for the pen tip. Set this value to avoid hiding the virtual pen tip under the pyhsical pen.",
	util::_default_value    = 0);

logger::LogChannel backendviewlog("backendviewlog", "[BackendView] ");

BackendView::BackendView() :
	_lastPen(0, 0),
	_gestureStartCenter(0, 0),
	_gestureStartDistance(0),
	_backgroundPainterStopped(false),
	_backgroundThread(boost::bind(&BackendView::cleanDirtyAreas, this)),
	_mode(Nothing),
	_penOffset(optionPenOffsetX.as<int>(), optionPenOffsetY.as<int>()) {

	registerInput(_document, "document");
	registerInput(_tools, "tools");
	registerOutput(_painter, "painter");

	_document.registerBackwardCallback(&BackendView::onDocumentChangedArea, this);
	_document.registerBackwardCallback(&BackendView::onStrokePointAdded, this);
	_document.registerBackwardCallback(&BackendView::onSelectionMoved, this);
	_tools.registerBackwardCallback(&BackendView::onToolsChangedArea, this);
	_tools.registerBackwardCallback(&BackendView::onLassoPointAdded, this);

	_painter.registerForwardSlot(_contentChanged);
	_painter.registerForwardSlot(_fullscreen);

	// establish pointer signal filter
	PointerSignalFilter::filterBackward(_painter, _document, this);

	_painter.registerForwardCallback(&BackendView::onMouseMove, this);
	_painter.registerForwardCallback(&BackendView::onPenMove, this);
	_painter.registerForwardCallback(&BackendView::onPenIn, this);
	_painter.registerForwardCallback(&BackendView::onPenOut, this);
	_painter.registerForwardCallback(&BackendView::onFingerDown, this);
	_painter.registerForwardCallback(&BackendView::onFingerMove, this);
	_painter.registerForwardCallback(&BackendView::onFingerUp, this);
}

BackendView::~BackendView() {

	LOG_DEBUG(backendviewlog) << "tearing down background rendering thread" << std::endl;

	_backgroundPainterStopped = true;
	_backgroundThread.join();

	LOG_DEBUG(backendviewlog) << "background rendering thread stopped" << std::endl;
}

void
BackendView::updateOutputs() {

	_painter->setDocument(_document);
	_painter->setTools(_tools);

	_contentChanged();
}

bool
BackendView::filter(gui::PointerSignal& signal) {

	signal.position = _painter->screenToDocument(signal.position + _penOffset);
	return true;
}

void
BackendView::onPenMove(const gui::PenMove& signal) {

	_lastPen = signal.position;
	_painter->setCursorPosition(signal.position + _penOffset);
	_contentChanged();
}

void
BackendView::onMouseMove(const gui::MouseMove& signal) {

	// the mouse cursor gives hints about the position of the pen (which the 
	// PenIn cannot tell us)
	_lastPen = signal.position + _penOffset;
}

void
BackendView::onPenIn(const gui::PenIn& /*signal*/) {

	LOG_ALL(backendviewlog) << "the pen came close to the screen" << std::endl;
	_penClose = true;
}

void
BackendView::onPenOut(const gui::PenOut& /*signal*/) {

	LOG_ALL(backendviewlog) << "the pen moved away from the screen" << std::endl;
	_penClose = false;
}

void
BackendView::onFingerDown(const gui::FingerDown& signal) {

	if (signal.processed || locked(signal.timestamp, signal.position))
		return;

	addFinger(signal);
}

void
BackendView::onFingerMove(const gui::FingerMove& signal) {

	if (signal.processed)
		return;

	LOG_ALL(backendviewlog) << "a finger is moved" << std::endl;

	// get the moving finger
	std::map<int, gui::FingerSignal>::iterator i = _fingerDown.find(signal.id);
	if (i == _fingerDown.end()) {

		LOG_ALL(backendviewlog) << "got a move from unseen (or ignored) finger " << signal.id << std::endl;
		return;
	}

	// is there a reason not to accept the drag and zoom?
	if (locked(signal.timestamp, signal.position)) {

		LOG_ALL(backendviewlog) << "finger " << signal.id << " is locked -- erase it" << std::endl;
		removeFinger(signal.id, signal.timestamp);
		return;
	}

	// update the position of the finger
	i->second.position = signal.position;

	if (_mode == WindowRequests) {

		LOG_ALL(backendviewlog) << "I am in window request mode (number of fingers down == 3)" << std::endl;

		// get the amount by which the fingers where moved
		util::point<DocumentPrecision> moved = getFingerCenter() - _gestureStartCenter;

		LOG_ALL(backendviewlog) << "moved by " << moved << " (" << getFingerCenter() << " - " << _gestureStartCenter << ")" << std::endl;

		DocumentPrecision threshold = WindowRequestThreshold;

		// moved upwards
		if (moved.y < -threshold) {

			LOG_ALL(backendviewlog) << "requesting fullscreen" << std::endl;
			_fullscreen(gui::WindowFullscreen(true));
		}

		// moved downwards
		if (moved.y > threshold) {

			LOG_ALL(backendviewlog) << "requesting no fullscreen" << std::endl;
			_fullscreen(gui::WindowFullscreen(false));
		}
	}

	if (_mode == StartZooming) {

		util::point<DocumentPrecision> moved = getFingerCenter() - _gestureStartCenter;

		// finger center must not move
		if (sqrt(moved.x*moved.x + moved.y*moved.y)/_gestureStartDistance > ZoomMaxCenterMoveRatio) {

			clearFingers();
			return;
		}

		// fingers must have minimal distance to start
		if (_gestureStartDistance < ZoomThreshold) {

			_gestureStartDistance = getFingerDistance();
			return;
		}

		double zoomed = getFingerDistance()/_gestureStartDistance;

		// scale change shall not be too big
		if (zoomed > 1.5 || zoomed < 0.75) {

			_gestureStartDistance = getFingerDistance();
			return;
		}

		// scale change has to have minimal value
		if (std::abs(1.0 - zoomed) > ZoomThreshold) {

			_gestureStartDistance = getFingerDistance();
			_mode = Zooming;
		}
	}

	if (_mode == Zooming) {

		LOG_ALL(backendviewlog) << "I am in zooming mode (number of fingers down == 2)" << std::endl;

		// the previous distance between the fingers
		DocumentPrecision previousDistance = _gestureStartDistance;

		LOG_ALL(backendviewlog) << "previous finger distance was " << previousDistance << std::endl;

		DocumentPrecision distance = getFingerDistance();

		LOG_ALL(backendviewlog) << "current finger distance is " << distance << std::endl;
		LOG_ALL(backendviewlog) << "zooming by " << (distance/previousDistance) << " with center at " << getFingerCenter() << std::endl;

		_painter->zoom(distance/previousDistance, getFingerCenter());

		// remember last distance
		_gestureStartDistance = distance;

		_contentChanged();

		return;
	}

	if (_mode == StartDragging) {

		// don't wait too long
		if (signal.timestamp - _gestureStartTime > DragTimeout) {

			clearFingers();
			return;
		}

		util::point<DocumentPrecision> moved = getFingerCenter() - _gestureStartCenter;

		if (moved.x*moved.x + moved.y*moved.y > DragThreshold2) {

			_gestureStartCenter = getFingerCenter();
			_mode = Dragging;
		}
	}

	if (_mode == Dragging) {

		// determine drag, let each finger contribute with equal weight
		util::point<DocumentPrecision> drag = (1.0/_fingerDown.size())*(signal.position - _gestureStartCenter);

		// set drag
		_painter->drag(drag);

		// remember last dragging position
		_gestureStartCenter = getFingerCenter();

		_contentChanged();

		return;
	}
}

void
BackendView::onFingerUp(const gui::FingerUp& signal) {

	if (signal.processed)
		return;

	removeFinger(signal.id, signal.timestamp);
}

void
BackendView::addFinger(const gui::FingerDown& signal) {

	LOG_ALL(backendviewlog) << "a finger was put down (" << _fingerDown.size() << " fingers now)" << std::endl;

	_fingerDown[signal.id] = signal;
	initGesture(signal.timestamp);
	setMode();
}

void
BackendView::removeFinger(unsigned int id, unsigned long timestamp) {

	LOG_ALL(backendviewlog) << "finger " << id << " finger was moved up" << std::endl;

	std::map<int, gui::FingerSignal>::iterator i = _fingerDown.find(id);

	// is this one of the fingers we are listening to?
	if (i != _fingerDown.end()) {

		LOG_ALL(backendviewlog) << "this is one of the fingers I am listening to" << std::endl;

		_fingerDown.erase(i);
		_painter->prepareDrawing();
		_contentChanged();
		initGesture(timestamp);
		setMode();

	} else {

		LOG_ALL(backendviewlog) << "this finger is ignored" << std::endl;
	}
}

void
BackendView::clearFingers() {

	_fingerDown.clear();
	setMode();
}

void
BackendView::setMode() {

	switch (_fingerDown.size()) {

		case 1:
			_mode = StartDragging;
			break;

		case 2:
			_mode = StartZooming;
			break;

		case 3:
			_mode = WindowRequests;
			break;

		default:
			_mode = Nothing;
			break;
	}
}

void
BackendView::initGesture(unsigned long timestamp) {

	_gestureStartCenter   = getFingerCenter();
	_gestureStartDistance = getFingerDistance();
	_gestureStartTime     = timestamp;
}

void
BackendView::onDocumentChangedArea(const ChangedArea& signal) {

	LOG_ALL(backendviewlog) << "document area " << signal.area << " changed" << std::endl;

	_painter->markDirty(signal.area);
	_contentChanged();
}

void
BackendView::onToolsChangedArea(const ChangedArea& signal) {

	LOG_ALL(backendviewlog) << "tools area " << signal.area << " changed" << std::endl;

	_contentChanged();
}

void
BackendView::onStrokePointAdded(const StrokePointAdded& signal) {

	LOG_ALL(backendviewlog) << "a stroke point was added -- initiate a redraw" << std::endl;

	_painter->contentAdded(signal.area);
	_contentChanged();
}

void
BackendView::onSelectionMoved(const SelectionMoved& signal) {

	_painter->moveSelection(signal);

	_contentChanged();
}

void
BackendView::onLassoPointAdded(const LassoPointAdded& /*signal*/) {

	LOG_ALL(backendviewlog) << "a lasso point was added -- initiate a redraw" << std::endl;

	_contentChanged();
}

DocumentPrecision
BackendView::getFingerDistance() {

	if (_fingerDown.size() != 2)
		return 0;

	std::map<int, gui::FingerSignal>::const_iterator i = _fingerDown.begin();

	util::point<DocumentPrecision> diff = i->second.position;
	i++;
	diff -= i->second.position;

	return sqrt(diff.x*diff.x + diff.y*diff.y);
}

util::point<DocumentPrecision>
BackendView::getFingerCenter() {

	util::point<DocumentPrecision> center(0, 0);
	for (std::map<int, gui::FingerSignal>::const_iterator i = _fingerDown.begin(); i != _fingerDown.end(); i++)
		center += i->second.position;

	return center/_fingerDown.size();
}

bool
BackendView::locked(unsigned long /*now*/, const util::point<DocumentPrecision>& position) {

	// if there is a pen, allow only dragging and moving from an area that is 
	// clearly not the palm (of a right-handed person)
	if (_penClose) {

		LOG_ALL(backendviewlog) << "the pen is close to the screen" << std::endl;

		if (position.x < _lastPen.x) {

			return false;

		} else {

			LOG_ALL(backendviewlog) << "the pen is locking the document" << std::endl;
			return true;
		}
	}

	return false;
}

void
BackendView::cleanDirtyAreas() {

	boost::timer::cpu_timer timer;

	const boost::timer::nanosecond_type NanosBusyWait = 100000LL;     // 1/10000th of a second
	const boost::timer::nanosecond_type NanosIdleWait = 1000000000LL; // 1/1th of a second

	bool isClean = false;

	while (!_backgroundPainterStopped) {

		// was there something to clean?
		if (_painter && _painter->cleanDirtyAreas(2)) {

			isClean = false;

		// there was nothing to clean
		} else {

			if (!isClean)
				_contentChanged();
			isClean = true;
		}

		boost::timer::cpu_times const elapsed(timer.elapsed());

		boost::timer::nanosecond_type waitAtLeast = (isClean ? NanosIdleWait : NanosBusyWait);

		if (elapsed.wall <= waitAtLeast)
			usleep((waitAtLeast - elapsed.wall)/1000);

		timer.stop();
		timer.start();
	}
}
