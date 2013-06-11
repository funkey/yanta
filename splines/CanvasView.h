#ifndef SPLINES_CANVAS_VIEW_H__
#define SPLINES_CANVAS_VIEW_H__

#include <boost/thread.hpp>

#include <pipeline/all.h>
#include <gui/GuiSignals.h>
#include <gui/PointerSignalFilter.h>

#include "CanvasPainter.h"
#include "Strokes.h"

class CanvasView : public pipeline::SimpleProcessNode<>, public gui::PointerSignalFilter {

public:

	CanvasView();

	~CanvasView();

private:

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

	double getFingerDistance();

	util::point<double> getFingerCenter();

	void cleanDirtyAreas();

	/**
	 * Returns true if there was recent pen activity and finger movements should 
	 * be ignored.
	 */
	bool locked(unsigned long now, const util::point<double>& position);

	pipeline::Input<Strokes>        _strokes;
	pipeline::Output<CanvasPainter> _painter;

	// TODO: do we need that?
	signals::Slot<const gui::ContentChanged> _contentChanged;

	// remember the last position of each finger
	std::map<int, gui::FingerSignal> _fingerDown;

	// is the pen in the proximity of the screen?
	bool _penClose;

	// the last known position of the pen
	util::point<double> _lastPen;

	// used to stop the background rendering thread
	bool _backgroundPainterStopped;

	// the background rendering thread keeping dirty regions clean
	boost::thread _backgroundThread;
};

#endif // SPLINES_CANVAS_VIEW_H__

