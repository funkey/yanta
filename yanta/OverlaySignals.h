#ifndef YANTA_OVERLAY_SIGNALS_H__
#define YANTA_OVERLAY_SIGNALS_H__

#include <util/rect.hpp>
#include <signals/Signal.h>

/**
 * Base class for all canvas signals.
 */
class OverlaySignal : public signals::Signal {};

/**
 * Signal to send whenever an area of the canvas changed in a way that a 
 * complete redraw in this area is neccessary.
 */
class OverlayChangedArea : public OverlaySignal {

public:

	OverlayChangedArea() :
		area(0, 0, 0, 0) {}

	OverlayChangedArea(const util::rect<CanvasPrecision>& area_) :
		area(area_) {}

	util::rect<CanvasPrecision> area;
};

/**
 * Signal to send when just a point was added to the lasso.
 */
class LassoPointAdded : public OverlaySignal {};

#endif // YANTA_OVERLAY_SIGNALS_H__

