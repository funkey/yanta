#ifndef YANTA_OVERLAY_OBJECT_H__
#define YANTA_OVERLAY_OBJECT_H__

#include <util/rect.hpp>
#include <document/Precision.h>
#include <document/Transformable.h>
#include "OverlayObjectVisitor.h"

class OverlayObject : public Transformable<DocumentPrecision> {

public:

	virtual void visit(OverlayObjectVisitor& visitor) = 0;
};

#endif // YANTA_OVERLAY_OBJECT_H__

