#ifndef YANTA_TOOL_SIGNALS_H__
#define YANTA_TOOL_SIGNALS_H__

#include <Signals.h>
#include "PenMode.h"

/**
 * Signal to send when just a point was added to the lasso.
 */
class LassoPointAdded : public ChangedArea {

public:

	LassoPointAdded() :
		ChangedArea() {}

	LassoPointAdded(const util::point<int>& start, const util::point<int>& point) :
		ChangedArea(
				util::rect<int>(
						std::min(start.x, point.x),
						std::min(start.y, point.y),
						std::max(start.x, point.x),
						std::max(start.y, point.y))) {}
};

/**
 * Signal to send when the pen mode changed.
 */
class PenModeChanged : public signals::Signal {

public:

	PenModeChanged() {}

	PenModeChanged(const PenMode& newMode) :
		_newMode(newMode) {}

	const PenMode& getNewMode() const { return _newMode; }

private:

	PenMode _newMode;
};

#endif // YANTA_TOOL_SIGNALS_H__

