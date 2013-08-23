#ifndef YANTA_DOCUMENT_SIGNALS_H__
#define YANTA_DOCUMENT_SIGNALS_H__

#include <Signals.h>

/**
 * Signal to indicate that the document changed incrementally.
 */
class ContentAdded : public ChangedArea {

public:

	ContentAdded() : ChangedArea() {}

	ContentAdded(const util::rect<DocumentPrecision>& area_) :
		ChangedArea(area_) {}
};

/**
 * Signal to send when just a stroke point was added to the current stroke.
 */
class StrokePointAdded : public ContentAdded {

public:

	StrokePointAdded() : ContentAdded() {}

	StrokePointAdded(const util::rect<DocumentPrecision>& area_) :
		ContentAdded(area_) {}
};

/**
 * Signal to send when a selection was moved.
 */
class SelectionMoved : public ChangedArea {

public:

	SelectionMoved() :
		ChangedArea(),
		index(0), shift(0, 0) {}

	SelectionMoved(
			const util::rect<DocumentPrecision>& area_,
			unsigned int index_,
			const util::point<DocumentPrecision>& shift_) :
		ChangedArea(area_),
		index(index_),
		shift(shift_) {}

	unsigned int index;

	util::point<DocumentPrecision> shift;
};

#endif // YANTA_DOCUMENT_SIGNALS_H__

