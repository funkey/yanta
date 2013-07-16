#ifndef YANTA_OVERLAY_OBJECT_H__
#define YANTA_OVERLAY_OBJECT_H__

#include <util/rect.hpp>
#include "OverlayObjectVisitor.h"
#include "Precision.h"

class OverlayObject {

public:

	virtual const util::rect<CanvasPrecision>& getRoi() const = 0;

	virtual void visit(OverlayObjectVisitor& visitor) = 0;
};

#endif // YANTA_OVERLAY_OBJECT_H__

