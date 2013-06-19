#ifndef YANTA_CANVAS_SIGNALS_H__
#define YANTA_CANVAS_SIGNALS_H__

#include <signals/Signal.h>
#include <util/rect.hpp>

class ChangedArea : public signals::Signal {

public:

	ChangedArea() :
		area(0, 0, 0, 0) {}

	ChangedArea(const util::rect<Canvas::Precision>& area_) :
		area(area_) {}

	util::rect<Canvas::Precision> area;
};

#endif // YANTA_CANVAS_SIGNALS_H__

