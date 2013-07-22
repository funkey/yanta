#ifndef YANTA_DOCUMENT_SIGNALS_H__
#define YANTA_DOCUMENT_SIGNALS_H__

#include <signals/Signal.h>
#include <util/rect.hpp>

#include "Precision.h"

/**
 * Base class for all document signals.
 */
class DocumentSignal : public signals::Signal {};

/**
 * Signal to send whenever an area of the document changed in a way that a 
 * complete redraw in this area is neccessary.
 */
class DocumentChangedArea : public DocumentSignal {

public:

	DocumentChangedArea() :
		area(0, 0, 0, 0) {}

	DocumentChangedArea(const util::rect<DocumentPrecision>& area_) :
		area(area_) {}

	util::rect<DocumentPrecision> area;
};

/**
 * Signal to send when just a stroke point was added to the current stroke.
 */
class StrokePointAdded : public DocumentSignal {};

#endif // YANTA_DOCUMENT_SIGNALS_H__

