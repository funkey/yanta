#ifndef YANTA_SIGNALS_H__
#define YANTA_SIGNALS_H__

#include <signals/Signal.h>
#include <util/rect.hpp>

#include <document/Precision.h>

/**
 * Signal to send whenever an area of the document changed in a way that a 
 * complete redraw in this area is neccessary.
 */
class ChangedArea : public signals::Signal {

public:

	ChangedArea() :
		area(0, 0, 0, 0) {}

	ChangedArea(const util::rect<DocumentPrecision>& area_) :
		area(area_) {}

	util::rect<DocumentPrecision> area;
};


#endif // YANTA_SIGNALS_H__

