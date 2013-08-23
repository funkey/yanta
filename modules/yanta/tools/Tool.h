#ifndef YANTA_TOOL_H__
#define YANTA_TOOL_H__

#include <SkCanvas.h>

#include <document/DocumentElement.h>

/**
 * Base class for tools. Tools are DocumentElements, since they can be located 
 * in the document and have a bounding box as well. In contrast to regular 
 * DocumentElements they are, however, not owned by the document, but shared 
 * between the Backend and the Document.
 */
class Tool : public DocumentElement {

public:

	YANTA_TREE_VISITABLE()

	/**
	 * Draw the tool on the provided skia canvas.
	 */
	virtual void draw(SkCanvas& canvas) = 0;
};

#endif // YANTA_TOOL_H__

