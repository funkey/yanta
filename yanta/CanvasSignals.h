#ifndef YANTA_CANVAS_SIGNALS_H__
#define YANTA_CANVAS_SIGNALS_H__

#include <signals/Signal.h>
#include <util/rect.hpp>

/**
 * Base class for all canvas signals.
 */
class CanvasSignal : public signals::Signal {};

/**
 * Signal to send whenever an area of the canvas changed in a way that a 
 * complete redraw in this area is neccessary.
 */
class ChangedArea : public CanvasSignal {

public:

	ChangedArea() :
		area(0, 0, 0, 0) {}

	ChangedArea(const util::rect<CanvasPrecision>& area_) :
		area(area_) {}

	util::rect<CanvasPrecision> area;
};

/**
 * Signal to send when just a stroke point was added to the current stroke.
 */
class StrokePointAdded : public CanvasSignal {};

#endif // YANTA_CANVAS_SIGNALS_H__

