#ifndef YANTA_SELECTION_H__
#define YANTA_SELECTION_H__

#include "OverlayObject.h"
#include "Page.h"
#include "Stroke.h"

// forward declarations
class Path;
class Canvas;

/**
 * A container of strokes that are currently selected.
 */
class Selection : public OverlayObject {

public:

	/**
	 * Create a selection from a path and a canvas. Every canvas object that is 
	 * fully contained in the path will be added to the selection and removed 
	 * from the canvas.
	 */
	static Selection CreateFromPath(const Path& path, Canvas& canvas);

	Selection();

	/**
	 * Add a stroke to the selection.
	 */
	void addStroke(const Page& page, const Stroke& stroke);

	void visit(OverlayObjectVisitor& visitor) { visitor.processSelection(*this); }

private:

	std::vector<Stroke> _strokes;
};

#endif // YANTA_SELECTION_H__

