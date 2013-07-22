#ifndef YANTA_OVERLAY_H__
#define YANTA_OVERLAY_H__

#include <vector>

#include <pipeline/Data.h>

#include "OverlayObject.h"

/**
 * A collection of overlay objects to draw on top of a Document, like lasso 
 * selections, graphical hints, selection outlints, etc.
 */
class Overlay : public pipeline::Data {

public:

	unsigned int numObjects() const { return _objects.size(); }

	OverlayObject& getObject(unsigned int i) { return *_objects[i]; }
	const OverlayObject& getObject(unsigned int i) const { return *_objects[i]; }

	void add(boost::shared_ptr<OverlayObject> overlayObject) { _objects.push_back(overlayObject); }

	void remove(boost::shared_ptr<OverlayObject> overlayObject) {

		std::vector<boost::shared_ptr<OverlayObject> >::iterator i = std::find(_objects.begin(), _objects.end(), overlayObject);

		if (i != _objects.end())
			_objects.erase(i);
	
		return;
	}

private:

	std::vector<boost::shared_ptr<OverlayObject> > _objects;
};

#endif // YANTA_OVERLAY_H__

