#ifndef YANTA_GUI_RASTERIZER_H__
#define YANTA_GUI_RASTERIZER_H__

#include <util/rect.hpp>
#include <document/Precision.h>
#include "Quality.h"

// forward declaration
class SkCanvas;

class Rasterizer {

public:

	Rasterizer() : _quality(Auto) {}

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

	/**
	 * Set the quality of this rasterizer.
	 */
	void setQuality(Quality quality) { _quality = quality; }

	/**
	 * Get the quality of this rasterizer.
	 */
	inline Quality getQuality() { return _quality; }

private:

	// the level of quality to rasterize with
	Quality _quality;
};

#endif // YANTA_GUI_RASTERIZER_H__

