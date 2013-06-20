#include <util/Logger.h>
#include "CanvasView.h"

logger::LogChannel canvasviewlog("canvasviewlog", "[CanvasView] ");

CanvasView::CanvasView() :
	_lastPen(0, 0),
	_gestureStartCenter(0, 0),
	_gestureStartDistance(0),
	_backgroundPainterStopped(false),
	_backgroundThread(boost::bind(&CanvasView::cleanDirtyAreas, this)),
	_mode(Nothing) {

	registerInput(_canvas, "canvas");
	registerOutput(_painter, "painter");

	_canvas.registerBackwardCallback(&CanvasView::onChangedArea, this);

	_painter.registerForwardSlot(_contentChanged);
	_painter.registerForwardSlot(_fullscreen);

	// establish pointer signal filter
	PointerSignalFilter::filterBackward(_painter, _canvas, this);

	_painter.registerForwardCallback(&CanvasView::onMouseMove, this);
	_painter.registerForwardCallback(&CanvasView::onPenMove, this);
	_painter.registerForwardCallback(&CanvasView::onPenIn, this);
	_painter.registerForwardCallback(&CanvasView::onPenOut, this);
	_painter.registerForwardCallback(&CanvasView::onFingerDown, this);
	_painter.registerForwardCallback(&CanvasView::onFingerMove, this);
	_painter.registerForwardCallback(&CanvasView::onFingerUp, this);
}

CanvasView::~CanvasView() {

	LOG_DEBUG(canvasviewlog) << "tearing down background rendering thread" << std::endl;

	_backgroundPainterStopped = true;
	_backgroundThread.join();

	LOG_DEBUG(canvasviewlog) << "background rendering thread stopped" << std::endl;
}

void
CanvasView::updateOutputs() {

	_painter->setCanvas(_canvas);

	_contentChanged();
}

bool
CanvasView::filter(gui::PointerSignal& signal) {

	signal.position = _painter->screenToCanvas(signal.position);
	return true;
}

void
CanvasView::onPenMove(const gui::PenMove& signal) {

	_lastPen = signal.position;
	_painter->setCursorPosition(signal.position);
	_contentChanged();
}

void
CanvasView::onMouseMove(const gui::MouseMove& signal) {

	// the mouse cursor gives hints about the position of the pen (which the 
	// PenIn cannot tell us)
	_lastPen = signal.position;
}

void
CanvasView::onPenIn(const gui::PenIn& /*signal*/) {

	LOG_ALL(canvasviewlog) << "the pen came close to the screen" << std::endl;
	_penClose = true;
}

void
CanvasView::onPenOut(const gui::PenOut& /*signal*/) {

	LOG_ALL(canvasviewlog) << "the pen moved away from the screen" << std::endl;
	_penClose = false;
}

void
CanvasView::onFingerDown(const gui::FingerDown& signal) {

	if (signal.processed || locked(signal.timestamp, signal.position))
		return;

	addFinger(signal);
}

void
CanvasView::onFingerMove(const gui::FingerMove& signal) {

	if (signal.processed)
		return;

	LOG_ALL(canvasviewlog) << "a finger is moved" << std::endl;

	// get the moving finger
	std::map<int, gui::FingerSignal>::iterator i = _fingerDown.find(signal.id);
	if (i == _fingerDown.end()) {

		LOG_ALL(canvasviewlog) << "got a move from unseen (or ignored) finger " << signal.id << std::endl;
		return;
	}

	// is there a reason not to accept the drag and zoom?
	if (locked(signal.timestamp, signal.position)) {

		LOG_ALL(canvasviewlog) << "finger " << signal.id << " is locked -- erase it" << std::endl;
		removeFinger(signal.id);
		return;
	}

	// update the position of the finger
	i->second.position = signal.position;

	if (_mode == WindowRequests) {

		LOG_ALL(canvasviewlog) << "I am in window request mode (number of fingers down == 3)" << std::endl;

		// get the amount by which the fingers where moved
		util::point<CanvasPrecision> moved = getFingerCenter() - _gestureStartCenter;

		LOG_ALL(canvasviewlog) << "moved by " << moved << " (" << getFingerCenter() << " - " << _gestureStartCenter << ")" << std::endl;

		CanvasPrecision threshold = WindowRequestThreshold;

		// moved upwards
		if (moved.y < -threshold) {

			LOG_ALL(canvasviewlog) << "requesting fullscreen" << std::endl;
			_fullscreen(gui::WindowFullscreen(true));
		}

		// moved downwards
		if (moved.y > threshold) {

			LOG_ALL(canvasviewlog) << "requesting no fullscreen" << std::endl;
			_fullscreen(gui::WindowFullscreen(false));
		}
	}

	if (_mode == StartZooming) {

		if (_gestureStartDistance < ZoomThreshold) {

			_gestureStartDistance = getFingerDistance();
			return;
		}

		double zoomed = getFingerDistance()/_gestureStartDistance;

		if (std::abs(1.0 - zoomed) > ZoomThreshold)
			_mode = Zooming;
	}

	if (_mode == Zooming) {

		LOG_ALL(canvasviewlog) << "I am in zooming mode (number of fingers down == 2)" << std::endl;

		// the previous distance between the fingers
		CanvasPrecision previousDistance = _gestureStartDistance;

		LOG_ALL(canvasviewlog) << "previous finger distance was " << previousDistance << std::endl;

		CanvasPrecision distance = getFingerDistance();

		LOG_ALL(canvasviewlog) << "current finger distance is " << distance << std::endl;
		LOG_ALL(canvasviewlog) << "zooming by " << (distance/previousDistance) << " with center at " << getFingerCenter() << std::endl;

		_painter->zoom(distance/previousDistance, getFingerCenter());

		// remember last distance
		_gestureStartDistance = distance;

		_contentChanged();

		return;
	}

	if (_mode == StartDragging) {

		util::point<CanvasPrecision> moved = getFingerCenter() - _gestureStartCenter;

		if (moved.x*moved.x + moved.y*moved.y > DragThreshold2)
			_mode = Dragging;
	}

	if (_mode == Dragging) {

		// determine drag, let each finger contribute with equal weight
		util::point<CanvasPrecision> drag = (1.0/_fingerDown.size())*(signal.position - _gestureStartCenter);

		// set drag
		_painter->drag(drag);

		// remember last dragging position
		_gestureStartCenter = getFingerCenter();

		_contentChanged();

		return;
	}
}

void
CanvasView::onFingerUp(const gui::FingerUp& signal) {

	if (signal.processed)
		return;

	removeFinger(signal.id);
}

void
CanvasView::addFinger(const gui::FingerDown& signal) {

	LOG_ALL(canvasviewlog) << "a finger was put down (" << _fingerDown.size() << " fingers now)" << std::endl;

	_fingerDown[signal.id] = signal;
	initGesture();
	setMode();
}

void
CanvasView::removeFinger(unsigned int id) {

	LOG_ALL(canvasviewlog) << "finger " << id << " finger was moved up" << std::endl;

	std::map<int, gui::FingerSignal>::iterator i = _fingerDown.find(id);

	// is this one of the fingers we are listening to?
	if (i != _fingerDown.end()) {

		LOG_ALL(canvasviewlog) << "this is one of the fingers I am listening to" << std::endl;

		_fingerDown.erase(i);
		_painter->prepareDrawing();
		_contentChanged();
		initGesture();
		setMode();

	} else {

		LOG_ALL(canvasviewlog) << "this finger is ignored" << std::endl;
	}
}

void
CanvasView::setMode() {

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
CanvasView::initGesture() {

	_gestureStartCenter   = getFingerCenter();
	_gestureStartDistance = getFingerDistance();
}

void
CanvasView::onChangedArea(const ChangedArea& signal) {

	LOG_ALL(canvasviewlog) << "area " << signal.area << " changed" << std::endl;

	_painter->markDirty(signal.area);
	_contentChanged();
}

CanvasPrecision
CanvasView::getFingerDistance() {

	if (_fingerDown.size() != 2)
		return 0;

	std::map<int, gui::FingerSignal>::const_iterator i = _fingerDown.begin();

	util::point<CanvasPrecision> diff = i->second.position;
	i++;
	diff -= i->second.position;

	return sqrt(diff.x*diff.x + diff.y*diff.y);
}

util::point<CanvasPrecision>
CanvasView::getFingerCenter() {

	util::point<CanvasPrecision> center(0, 0);
	for (std::map<int, gui::FingerSignal>::const_iterator i = _fingerDown.begin(); i != _fingerDown.end(); i++)
		center += i->second.position;

	return center/_fingerDown.size();
}

bool
CanvasView::locked(unsigned long /*now*/, const util::point<CanvasPrecision>& position) {

	// if there is a pen, allow only dragging and moving from an area that is 
	// clearly not the palm (of a right-handed person)
	if (_penClose) {

		LOG_ALL(canvasviewlog) << "the pen is close to the screen" << std::endl;

		if (position.x < _lastPen.x) {

			return false;

		} else {

			LOG_ALL(canvasviewlog) << "the pen is locking the canvas" << std::endl;
			return true;
		}
	}

	return false;
}

void
CanvasView::cleanDirtyAreas() {

	while (!_backgroundPainterStopped) {

		if (_painter && _painter->cleanDirtyAreas(2))
			_contentChanged();
		else
			usleep(10*1000);
	}
}
