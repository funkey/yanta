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

	LassoPointAdded(const util::point<double>& start, const util::point<double>& point1, const util::point<double>& point2) :
		ChangedArea(
				util::rect<double>(
						std::min(start.x, std::min(point1.x, point2.x)),
						std::min(start.y, std::min(point1.y, point2.y)),
						std::max(start.x, std::max(point1.x, point2.x)),
						std::max(start.y, std::max(point1.y, point2.y)))) {}
};

#endif // YANTA_TOOL_SIGNALS_H__

