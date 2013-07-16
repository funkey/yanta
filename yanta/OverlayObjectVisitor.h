#ifndef YANTA_OVERLAY_OBJECT_VISITOR_H__
#define YANTA_OVERLAY_OBJECT_VISITOR_H__

// forward declaration
class Lasso;

class OverlayObjectVisitor {

public:

	virtual void processLasso(const Lasso& lasso) = 0;
};

#endif // YANTA_OVERLAY_OBJECT_VISITOR_H__
