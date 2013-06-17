#include <util/Logger.h>
#include "CanvasView.h"

logger::LogChannel canvasviewlog("canvasviewlog", "[CanvasView] ");

CanvasView::CanvasView() :
	_backgroundPainterStopped(false),
	_backgroundThread(boost::bind(&CanvasView::cleanDirtyAreas, this)) {

	registerInput(_strokes, "strokes");
	registerOutput(_painter, "painter");

	_strokes.registerBackwardCallback(&CanvasView::onChangedArea, this);

	_painter.registerForwardSlot(_contentChanged);

	// establish pointer signal filter
	PointerSignalFilter::filterBackward(_painter, _strokes, this);

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

	_painter->setStrokes(_strokes);

	// TODO: do we need that?
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

	LOG_ALL(canvasviewlog) << "a finger was put down (" << _fingerDown.size() << " fingers now)" << std::endl;

	_fingerDown[signal.id] = signal;
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

		// don't listen to this finger anymore
		_fingerDown.erase(i);

		// this is like moving a valid finger up
		_painter->prepareDrawing();
		_contentChanged();

		return;
	}

	if (_fingerDown.size() > 2)
		return;

	// get the previous position of the finger
	util::point<double>& previousPosition = i->second.position;

	// determine drag, let each finger contribute with equal weight
	util::point<double> moved = (1.0/_fingerDown.size())*(signal.position - previousPosition);

	// if two fingers are down, perform a zoom
	double previousDistance = 0;
	if (_fingerDown.size() == 2) {

		LOG_ALL(canvasviewlog) << "I am in zooming mode (number of fingers down == 2)" << std::endl;

		// the previous distance between the fingers
		previousDistance = getFingerDistance();

		LOG_ALL(canvasviewlog) << "previous finger distance was " << previousDistance << std::endl;
	}

	// update remembered position
	previousPosition = signal.position;

	// set drag
	_painter->drag(moved);

	// set zoom
	if (_fingerDown.size() == 2) {

		double distance = getFingerDistance();

		LOG_ALL(canvasviewlog) << "current finger distance is " << distance << std::endl;
		LOG_ALL(canvasviewlog) << "zooming by " << (distance/previousDistance) << " with center at " << getFingerCenter() << std::endl;

		_painter->zoom(distance/previousDistance, getFingerCenter());
	}

	_contentChanged();
}

void
CanvasView::onFingerUp(const gui::FingerUp& signal) {

	if (signal.processed)
		return;

	LOG_ALL(canvasviewlog) << "finger " << signal.id << " finger was moved up" << std::endl;

	std::map<int, gui::FingerSignal>::iterator i = _fingerDown.find(signal.id);

	// is this one of the fingers we are listening to?
	if (i != _fingerDown.end()) {

		LOG_ALL(canvasviewlog) << "this is one of the fingers I am listening to" << std::endl;

		_fingerDown.erase(i);
		_painter->prepareDrawing();
		_contentChanged();

	} else {

		LOG_ALL(canvasviewlog) << "this finger is ignored" << std::endl;
	}
}

void
CanvasView::onChangedArea(const ChangedArea& signal) {

	LOG_ALL(canvasviewlog) << "area " << signal.area << " changed" << std::endl;

	_painter->markDirty(signal.area);
	_contentChanged();
}

double
CanvasView::getFingerDistance() {

	if (_fingerDown.size() != 2)
		return 0;

	std::map<int, gui::FingerSignal>::const_iterator i = _fingerDown.begin();

	util::point<double> diff = i->second.position;
	i++;
	diff -= i->second.position;

	return sqrt(diff.x*diff.x + diff.y*diff.y);
}

util::point<double>
CanvasView::getFingerCenter() {

	util::point<double> center(0, 0);
	for (std::map<int, gui::FingerSignal>::const_iterator i = _fingerDown.begin(); i != _fingerDown.end(); i++)
		center += i->second.position;

	return center/_fingerDown.size();
}

bool
CanvasView::locked(unsigned long /*now*/, const util::point<double>& position) {

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
