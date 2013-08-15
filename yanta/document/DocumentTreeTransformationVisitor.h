#ifndef YANTA_DOCUMENT_DOCUMENT_TREE_TRANSFORMATION_VISITOR_H__
#define YANTA_DOCUMENT_DOCUMENT_TREE_TRANSFORMATION_VISITOR_H__

#include "DocumentTreeVisitor.h"
#include "DocumentElement.h"
#include "DocumentElementContainer.h"

/**
 * Base class for document tree visitors, that need to keep track of the 
 * accumulated transformations of the current path.
 */
class DocumentTreeTransformationVisitor : public DocumentTreeVisitor {

public:

	/**
	 * Get the current transformation of this visitor.
	 */
	const Transformation<DocumentPrecision>& getTransformation() { return _transformation; }

	/**
	 * Accumulate transformations.
	 */
	void enter(DocumentElement& element) {

		_transformation = element.getTransformation().applyTo(_transformation);

		LOG_ALL(documenttreetransformationvisitorlog) << "entering an element" << std::endl;
		LOG_ALL(documenttreetransformationvisitorlog) << "transformation is now " << _transformation << std::endl;
	}

	/**
	 * Restore the transformation.
	 */
	void leave(DocumentElement& element) {

		_transformation = element.getTransformation().getInverse().applyTo(_transformation);

		LOG_ALL(documenttreetransformationvisitorlog) << "leaving an element" << std::endl;
		LOG_ALL(documenttreetransformationvisitorlog) << "transformation is now " << _transformation << std::endl;
	}

private:

	Transformation<DocumentPrecision> _transformation;

	static logger::LogChannel documenttreetransformationvisitorlog;
};

#endif // YANTA_DOCUMENT_DOCUMENT_TREE_TRANSFORMATION_VISITOR_H__

