#ifndef YANTA_OVERLAY_OBJECT_H__
#define YANTA_OVERLAY_OBJECT_H__

#include <util/rect.hpp>
#include "OverlayObjectVisitor.h"
#include "Precision.h"
#include "Transformable.h"

class OverlayObject : public Transformable<CanvasPrecision> {

public:

	virtual void visit(OverlayObjectVisitor& visitor) = 0;
};

#endif // YANTA_OVERLAY_OBJECT_H__

