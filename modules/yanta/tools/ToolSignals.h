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

#endif // YANTA_TOOL_SIGNALS_H__

