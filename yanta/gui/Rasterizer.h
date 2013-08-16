#ifndef YANTA_GUI_RASTERIZER_H__
#define YANTA_GUI_RASTERIZER_H__

#include <util/rect.hpp>
#include <document/Precision.h>

// forward declaration
class SkCanvas;

class Rasterizer {

public:

	/**
	 * Draw on the given canvas within the given roi.
	 */
	virtual void draw(
			SkCanvas& canvas,
			const util::rect<DocumentPrecision>& roi) = 0;

	/**
	 * Enable or disable incremental drawing mode. Can be implemented by 
	 * subclasses for quick incremental updates.
	 */
	virtual void setIncremental(bool /*incremental*/) {};
};

#endif // YANTA_GUI_RASTERIZER_H__

