#ifndef YANTA_DOCUMENT_ELEMENT_CONTAINER_H__
#define YANTA_DOCUMENT_ELEMENT_CONTAINER_H__

#include <util/multi_container.hpp>

#include "DocumentElement.h"

/**
 * A document element holding an arbitrary number of different, compile-time 
 * fixed document element types. The types are passed as a typelist.
 */
template <typename Types>
class DocumentElementContainer : public DocumentElement, public multi_container<Types> {

public:

	/**
	 * Overrides the traverse function to pass the visitor to the elements of 
	 * this container.
	 */
	template <typename VisitorType>
	void traverse(VisitorType& visitor) {

		Traverser<VisitorType> traverser(visitor);
		multi_container<Types>::for_each(traverser);
	}

private:

	/**
	 * Functor to pass the visitor to the elements of this container.
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
};

#endif // YANTA_DOCUMENT_ELEMENT_CONTAINER_H__

