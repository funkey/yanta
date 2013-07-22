#ifndef YANTA_SELECTION_H__
#define YANTA_SELECTION_H__

#include <gui/OverlayObject.h>
#include "Page.h"
#include "Stroke.h"

// forward declarations
class Path;
class Document;

/**
 * A container of strokes that are currently selected.
 */
class Selection : public OverlayObject {

public:

	/**
	 * Create a selection from a path and a document. Every document object that is 
	 * fully contained in the path will be added to the selection and removed 
	 * from the document.
	 */
	static Selection CreateFromPath(const Path& path, Document& document);

	Selection(const StrokePoints& strokePoints);

	/**
	 * Add a stroke to the selection.
	 */
	void addStroke(const Page& page, const Stroke& stroke);

	Stroke& getStroke(unsigned int i) { return _strokes[i]; }
	const Stroke& getStroke(unsigned int i) const { return _strokes[i]; }

	unsigned int numStrokes() const { return _strokes.size(); }

	const StrokePoints& getStrokePoints() const { return _strokePoints; }

	/**
	 * Place the content of the selection on the document.
	 */
	void anchor(Document& document);

	void visit(OverlayObjectVisitor& visitor) { visitor.processSelection(*this); }

private:

	std::vector<Stroke> _strokes;
	const StrokePoints& _strokePoints;
};

#endif // YANTA_SELECTION_H__

