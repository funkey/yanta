#ifndef YANTA_CANVAS_VIEW_H__
#define YANTA_CANVAS_VIEW_H__

#include <boost/thread.hpp>

#include <pipeline/all.h>
#include <gui/GuiSignals.h>
#include <gui/PointerSignalFilter.h>
#include <gui/WindowSignals.h>

#include <document/Document.h>
#include <document/DocumentSignals.h>
#include <tools/Tools.h>
#include <tools/ToolSignals.h>
#include "BackendPainter.h"

class BackendView : public pipeline::SimpleProcessNode<>, public gui::PointerSignalFilter {

public:

	BackendView();

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
	 * pixel units and document units.
	 */
	bool filter(gui::PointerSignal&);

	// callbacks from user

	void onMouseMove(const gui::MouseMove& signal);

	void onPenMove(const gui::PenMove& signal);

	void onPenIn(const gui::PenIn& signal);

	void onPenOut(const gui::PenOut& signal);

	void onFingerDown(const gui::FingerDown& signal);

	void onFingerMove(const gui::FingerMove& signal);

	void onFingerUp(const gui::FingerUp& signal);


	// callbacks from backend for document

	void onDocumentChanged(const pipeline::Modified& signal);

	void onDocumentChangedArea(const ChangedArea& signal);

	void onStrokePointAdded(const StrokePointAdded& signal);


	// callbacks from backend for tools

	void onToolsChangedArea(const ChangedArea& signal);

	void onLassoPointAdded(const LassoPointAdded& signal);

	void onPenModeChanged(const PenModeChanged& signal);


	// callbacks from gui for painter

	void addFinger(const gui::FingerDown& signal);

	void removeFinger(unsigned int id, unsigned long timestamp);

	void clearFingers();

	void setMode();

	void initGesture(unsigned long timestamp);

	DocumentPrecision getFingerDistance();

	util::point<DocumentPrecision> getFingerCenter();

	/**
	 * Returns true if there was recent pen activity and finger movements should 
	 * be ignored.
	 */
	bool locked(unsigned long now, const util::point<DocumentPrecision>& position);

	// number of pixels to move at once before a window request is sent
	static const DocumentPrecision WindowRequestThreshold = 100;

	// min zoom change (in either direction) before zoom gets accepted
	static const DocumentPrecision ZoomThreshold          = 0.2;
	// min finger distance, before zoom gets accepted
	static const DocumentPrecision ZoomMinDistance        = 200;
	// max allowed movement of finger center (normalized by initial finger 
	// distance) before zoom gets accepted
	static const DocumentPrecision ZoomMaxCenterMoveRatio = 0.1;

	// max distance to travel before drag gets accepted
	static const DocumentPrecision DragThreshold2       = 50*50;
	// max time to hit the threshold before a drag gets accepted
	static const unsigned long DragTimeout              = 500;

	pipeline::Input<Document>        _document;
	pipeline::Input<Tools>           _tools;
	pipeline::Output<BackendPainter> _painter;

	signals::Slot<const gui::ContentChanged>   _contentChanged;
	signals::Slot<const gui::WindowFullscreen> _fullscreen;

	// remember the last position of each finger
	std::map<int, gui::FingerSignal> _fingerDown;

	// is the pen in the proximity of the screen?
	bool _penClose;

	// the last known position of the pen
	util::point<DocumentPrecision> _lastPen;

	// the center of the fingers at the beginning of a gesture (i.e., when the 
	// number of fingers on the screen changes)
	util::point<DocumentPrecision> _gestureStartCenter;
	DocumentPrecision              _gestureStartDistance;
	unsigned long                  _gestureStartTime;

	int _mode;

	// a small offset for the virtual pen tip to not be occluded by the pyhsical 
	// pen
	util::point<int> _penOffset;

	// indicates that the document should be reloaded
	bool _documentChanged;
};

#endif // YANTA_CANVAS_VIEW_H__

