#ifndef YANTA_DOCUMENT_TREE_VISITOR_H__
#define YANTA_DOCUMENT_TREE_VISITOR_H__

#include <util/Logger.h>
#include <util/typename.h>

#include <util/tree.h>
#include "DocumentElement.h"
#include "DocumentElementContainer.h"

/**
 * Base class for document tree visitors. Since the visitor's type is a template 
 * in the visitable elements, this base class is not really neccessary. It just 
 * provides a default callback for tree elements that we are not interested in.
 */
class DocumentTreeVisitor : public TreeVisitor {

public:

	/**
	 * Default visit method for DocumentElements.
	 */
	void visit(DocumentElement& e) { LOG_ALL(documenttreevisitorlog) << "[" << typeName(*this) << "] fallback called for " << typeName(e) << std::endl; }

	/**
	 * Default traverse method for DocumentElementContainers. Calls accept() on 
	 * each element of the container.
	 */
	template <typename Types, typename VisitorType>
	void traverse(DocumentElementContainer<Types>& container, VisitorType& visitor) {

		Traverser<VisitorType> traverser(visitor);
		container.for_each(traverser);
	}

	// fallback implementation
	using TreeVisitor::traverse;

protected:

	/**
	 * Functor to pass the visitor to each element of a container.
	 */
	template <typename VisitorType>
	class Traverser {

	public:

		Traverser(VisitorType& visitor) : _visitor(visitor) {}

		template <typename ElementType>
		void operator()(ElementType& element) {

			element.accept(_visitor);
		}

	private:

		VisitorType& _visitor;
	};

	static logger::LogChannel documenttreevisitorlog;
};

#endif // YANTA_DOCUMENT_TREE_VISITOR_H__

