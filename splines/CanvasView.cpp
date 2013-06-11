#include <util/Logger.h>
#include "CanvasView.h"

logger::LogChannel canvasviewlog("canvasviewlog", "[CanvasView] ");

CanvasView::CanvasView() :
	_backgroundPainterStopped(false),
	_backgroundThread(boost::bind(&CanvasView::cleanDirtyAreas, this)) {

	registerInput(_strokes, "strokes");
	registerOutput(_painter, "painter");

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
}

void
CanvasView::updateOutputs() {

	_painter->setStrokes(_strokes);

	// TODO: do we need that?
	_contentChanged();
}

bool
CanvasView::filter(gui::PointerSignal& signal) {

	signal.position = _painter->pixelToCanvas(signal.position);
	return true;
}

void
CanvasView::onPenMove(const gui::PenMove& signal) {

	LOG_ALL(canvasviewlog) << "the pen moved to " << signal.position << std::endl;

	_lastPen = signal.position;
}

void
CanvasView::onMouseMove(const gui::MouseMove& signal) {

	LOG_ALL(canvasviewlog) << "the mouse moved to " << signal.position << std::endl;

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

	LOG_ALL(canvasviewlog) << "a finger was put down (" << _fingerDown.size() << " fingers now)" << std::endl;

	_fingerDown[signal.id] = signal;
}

void
CanvasView::onFingerMove(const gui::FingerMove& signal) {

	LOG_ALL(canvasviewlog) << "a finger is moved" << std::endl;

	if (_fingerDown.size() > 2)
		return;

	// get the moving finger
	std::map<int, gui::FingerSignal>::iterator i = _fingerDown.find(signal.id);
	if (i == _fingerDown.end()) {

		LOG_ERROR(canvasviewlog) << "got a move from unseen finger " << signal.id << std::endl;
		return;
	}

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

	// is there a reason not to accept the drag and zoom?
	if (locked(signal.timestamp, signal.position))
		return;

	// set drag
	_painter->drag(moved);

	// set zoom
	if (_fingerDown.size() == 2) {

		double distance = getFingerDistance();

		LOG_ALL(canvasviewlog) << "current finger distance is " << distance << std::endl;
		LOG_ALL(canvasviewlog) << "zooming by " << (distance/previousDistance) << " with center at " << getFingerCenter() << std::endl;

		_painter->zoom(distance/previousDistance, getFingerCenter());
	}

	_contentChanged();//setDirty(_painter);
}

void
CanvasView::onFingerUp(const gui::FingerUp& signal) {

	std::map<int, gui::FingerSignal>::iterator i = _fingerDown.find(signal.id);
	if (i != _fingerDown.end())
		_fingerDown.erase(i);

	_painter->startIncrementalDrawing();
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
