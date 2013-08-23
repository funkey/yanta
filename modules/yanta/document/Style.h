#ifndef YANTA_STYLE_H__
#define YANTA_STYLE_H__

class Style {

public:

	Style() :
		_width(1.0),
		_red(0),
		_green(0),
		_blue(0),
		_alpha(255) {}

	inline void setWidth(double width) { _width = width; }

	inline double width() const { return _width; }

	inline void setColor(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha = 255) {

		_red   = red;
		_green = green;
		_blue  = blue;
		_alpha = alpha;
	}

	inline void setRed(unsigned char red)     { _red   = red; }
	inline void setGreen(unsigned char green) { _green = green; }
	inline void setBlue(unsigned char blue)   { _blue  = blue; }
	inline void setAlpha(unsigned char alpha) { _alpha = alpha; }

	inline unsigned char getRed()   const { return _red; }
	inline unsigned char getGreen() const { return _green; }
	inline unsigned char getBlue()  const { return _blue; }
	inline unsigned char getAlpha() const { return _alpha; }

private:

	double _width;
	unsigned char _red;
	unsigned char _green;
	unsigned char _blue;
	unsigned char _alpha;
};

#endif // YANTA_STYLE_H__

