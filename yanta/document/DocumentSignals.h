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

/**
 * Signal to send when a selection was moved.
 */
class SelectionMoved : public DocumentChangedArea {

public:

	SelectionMoved() :
		DocumentChangedArea(),
		index(0), shift(0, 0) {}

	SelectionMoved(
			const util::rect<DocumentPrecision>& area_,
			unsigned int index_,
			const util::point<DocumentPrecision>& shift_) :
		DocumentChangedArea(area_),
		index(index_),
		shift(shift_) {}

	unsigned int index;

	util::point<DocumentPrecision> shift;
};

#endif // YANTA_DOCUMENT_SIGNALS_H__

