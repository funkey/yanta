#ifndef YANTA_DOCUMENT_ELEMENT_H__
#define YANTA_DOCUMENT_ELEMENT_H__

#include <util/tree.h>

#include "Precision.h"
#include "Transformable.h"

class DocumentElement : public Transformable<DocumentPrecision>, public TreeNode {};

#endif // YANTA_DOCUMENT_ELEMENT_H__

