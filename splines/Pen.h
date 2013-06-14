#ifndef SPLINES_PEN_H__
#define SPLINES_PEN_H__

class Pen {

public:

	Pen() :
		_width(1.0) {}

	inline void setWidth(double width) { _width = width; }

	inline double width() const { return _width; }

private:

	double _width;
};

#endif // SPLINES_PEN_H__

