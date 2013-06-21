#ifndef YANTA_CANVAS_VIEW_H__
#define YANTA_CANVAS_VIEW_H__

#include <boost/thread.hpp>

#include <pipeline/all.h>
#include <gui/GuiSignals.h>
#include <gui/PointerSignalFilter.h>
#include <gui/WindowSignals.h>

#include "CanvasPainter.h"
#include "CanvasSignals.h"
#include "Canvas.h"

class CanvasView : public pipeline::SimpleProcessNode<>, public gui::PointerSignalFilter {

public:

	CanvasView();

	~CanvasView();

private:

	/**
	 * Based on the number of fingers on the screen, these are the different 
	 * modes of operation.
	 */
	enum Mode {

		Nothing,        // no finger
		StartDragging,  // one finger, movement below threshold
		Dragging,       // one finger, movement was once above threshold
		StartZooming,   // two fingers, movement below threshold
		Zooming,        // two fingers, movement was once above threshold
		WindowRequests  // three fingers
	};

	void updateOutputs();

	/**
	 * Filter callback for generic 2D input signals. Here, we translate between 
	 * pixel units and canvas units.
	 */
	bool filter(gui::PointerSignal&);

	void onMouseMove(const gui::MouseMove& signal);

	void onPenMove(const gui::PenMove& signal);

	void onPenIn(const gui::PenIn& signal);

	void onPenOut(const gui::PenOut& signal);

	void onFingerDown(const gui::FingerDown& signal);

	void onFingerMove(const gui::FingerMove& signal);

	void onFingerUp(const gui::FingerUp& signal);

	void onChangedArea(const ChangedArea& signal);

	void onStrokePointAdded(const StrokePointAdded& signal);

	void addFinger(const gui::FingerDown& signal);

	void removeFinger(unsigned int id);

	void setMode();

	void initGesture();

	CanvasPrecision getFingerDistance();

	util::point<CanvasPrecision> getFingerCenter();

	void cleanDirtyAreas();

	/**
	 * Returns true if there was recent pen activity and finger movements should 
	 * be ignored.
	 */
	bool locked(unsigned long now, const util::point<CanvasPrecision>& position);

	// number of pixels to move at once before a window request is sent
	static const CanvasPrecision WindowRequestThreshold = 100;
	static const CanvasPrecision ZoomThreshold          = 0.2;
	static const CanvasPrecision ZoomMinDistance        = 200;
	static const CanvasPrecision DragThreshold2         = 50*50;

	pipeline::Input<Canvas>        _canvas;
	pipeline::Output<CanvasPainter> _painter;

	signals::Slot<const gui::ContentChanged>   _contentChanged;
	signals::Slot<const gui::WindowFullscreen> _fullscreen;

	// remember the last position of each finger
	std::map<int, gui::FingerSignal> _fingerDown;

	// is the pen in the proximity of the screen?
	bool _penClose;

	// the last known position of the pen
	util::point<CanvasPrecision> _lastPen;

	// the center of the fingers at the beginning of a gesture (i.e., when the 
	// number of fingers on the screen changes)
	util::point<CanvasPrecision> _gestureStartCenter;
	CanvasPrecision              _gestureStartDistance;

	// used to stop the background rendering thread
	bool _backgroundPainterStopped;

	// the background rendering thread keeping dirty regions clean
	boost::thread _backgroundThread;

	int _mode;
};

#endif // YANTA_CANVAS_VIEW_H__

