#ifndef PEN_MODE_H__
#define PEN_MODE_H__

#include <document/Style.h>
#include "Erasor.h"

/**
 * A class to represent the current mode of the pen. This mode determines 
 * whether a stroke with the pen is used for erasing or drawing.
 */
class PenMode {

public:

	enum Mode {

		Draw,
		Erase,
		Lasso
	};

	inline void setMode(Mode mode) { _mode = mode; }

	inline Mode getMode() const { return _mode; }

	inline void setStyle(const Style& style) { _style = style; }

	inline const Style& getStyle() const { return _style; }
	inline Style& getStyle() { return _style; }

	inline void setErasorMode(Erasor::Mode mode) { _erasorMode = mode; }

	inline Erasor::Mode getErasorMode() { return _erasorMode; }

private:

	Mode  _mode;
	Style _style;

	Erasor::Mode _erasorMode;
};

#endif // PEN_MODE_H__

