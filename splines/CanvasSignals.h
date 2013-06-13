#ifndef SPLINES_CANVAS_SIGNALS_H__
#define SPLINES_CANVAS_SIGNALS_H__

#include <signals/Signal.h>
#include <util/rect.hpp>

class ChangedArea : public signals::Signal {

public:

	ChangedArea() :
		area(0, 0, 0, 0) {}

	ChangedArea(const util::rect<double>& area_) :
		area(area_) {}

	util::rect<double> area;
};

#endif // SPLINES_CANVAS_SIGNALS_H__

