#ifndef YANTA_DOCUMENT_ELEMENT_H__
#define YANTA_DOCUMENT_ELEMENT_H__

#include <util/tree.h>

#include "Precision.h"
#include "Transformable.h"

class DocumentElement : public Transformable<DocumentPrecision> {

public:

	/**
	 * Default implementation of the traverse method -- do nothing. Container 
	 * classes (i.e., nodes in the tree) should overwrite this method to pass 
	 * the visitor to children.
	 */
	template <typename VisitorType>
	void traverse(VisitorType& visitor) {}
};

#endif // YANTA_DOCUMENT_ELEMENT_H__

