#ifndef YANTA_DOCUMENT_ELEMENT_CONTAINER_H__
#define YANTA_DOCUMENT_ELEMENT_CONTAINER_H__

#include <util/multi_container.hpp>

#include "DocumentElement.h"

/**
 * A document element holding an arbitrary number of different, compile-time 
 * fixed document element types. The types are passed as a typelist.
 */
template <typename Types>
class DocumentElementContainer : public DocumentElement, public multi_container<Types> {};

#endif // YANTA_DOCUMENT_ELEMENT_CONTAINER_H__

