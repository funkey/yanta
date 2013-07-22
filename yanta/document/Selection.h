#ifndef YANTA_SELECTION_H__
#define YANTA_SELECTION_H__

#include <util/tree.h>
#include <util/typelist.h>

#include "DocumentElementContainer.h"
#include "Page.h"
#include "Stroke.h"

// forward declarations
class Path;
class Document;

// the types of elements in a selection
typedef TYPELIST_1(Stroke) SelectionElementTypes;

/**
 * A container of strokes that are currently selected.
 */
class Selection : public DocumentElementContainer<SelectionElementTypes> {

public:

	YANTA_TREE_VISITABLE();

	/**
	 * Create a selection from a path and a document. Every document object that is 
	 * fully contained in the path will be added to the selection and removed 
	 * from the document.
	 */
	static Selection CreateFromPath(const Path& path, Document& document);

	/**
	 * Create a new selection.
	 */
	Selection(const StrokePoints& strokePoints);

	/**
	 * Copy a selection.
	 */
	Selection& operator=(const Selection& other);

	/**
	 * Add a stroke to the selection.
	 */
	void addStroke(const Page& page, const Stroke& stroke);

	// TODO: delete
	Stroke& getStroke(unsigned int i) { return get<Stroke>(i); }
	const Stroke& getStroke(unsigned int i) const { return get<Stroke>(i); }
	unsigned int numStrokes() const { return size<Stroke>(); }

	// TODO: delete?
	const StrokePoints& getStrokePoints() const { return _strokePoints; }

	/**
	 * Place the content of the selection on the document.
	 */
	void anchor(Document& document);

private:

	const StrokePoints& _strokePoints;
};

#endif // YANTA_SELECTION_H__

