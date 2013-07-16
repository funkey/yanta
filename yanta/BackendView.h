#ifndef YANTA_CANVAS_VIEW_H__
#define YANTA_CANVAS_VIEW_H__

#include <boost/thread.hpp>

#include <pipeline/all.h>
#include <gui/GuiSignals.h>
#include <gui/PointerSignalFilter.h>
#include <gui/WindowSignals.h>

#include "BackendPainter.h"
#include "Canvas.h"
#include "CanvasSignals.h"
#include "Overlay.h"
#include "OverlaySignals.h"

class BackendView : public pipeline::SimpleProcessNode<>, public gui::PointerSignalFilter {

public:

	BackendView();

	~BackendView();

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

	void onCanvasChangedArea(const CanvasChangedArea& signal);

	void onOverlayChangedArea(const OverlayChangedArea& signal);

	void onStrokePointAdded(const StrokePointAdded& signal);

	void onLassoPointAdded(const LassoPointAdded& signal);

	void addFinger(const gui::FingerDown& signal);

	void removeFinger(unsigned int id, unsigned long timestamp);

	void clearFingers();

	void setMode();

	void initGesture(unsigned long timestamp);

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

	// min zoom change (in either direction) before zoom gets accepted
	static const CanvasPrecision ZoomThreshold          = 0.2;
	// min finger distance, before zoom gets accepted
	static const CanvasPrecision ZoomMinDistance        = 200;
	// max allowed movement of finger center (normalized by initial finger 
	// distance) before zoom gets accepted
	static const CanvasPrecision ZoomMaxCenterMoveRatio = 0.1;

	// max distance to travel before drag gets accepted
	static const CanvasPrecision DragThreshold2         = 50*50;
	// max time to hit the threshold before a drag gets accepted
	static const unsigned long DragTimeout              = 500;

	pipeline::Input<Canvas>          _canvas;
	pipeline::Input<Overlay>         _overlay;
	pipeline::Output<BackendPainter> _painter;

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
	unsigned long                _gestureStartTime;

	// used to stop the background rendering thread
	bool _backgroundPainterStopped;

	// the background rendering thread keeping dirty regions clean
	boost::thread _backgroundThread;

	int _mode;

	// a small offset for the virtual pen tip to not be occluded by the pyhsical 
	// pen
	util::point<int> _penOffset;
};

#endif // YANTA_CANVAS_VIEW_H__

