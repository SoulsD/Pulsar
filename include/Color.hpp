#ifndef COLOR_HPP__
#define COLOR_HPP__

#include <cmath>
#include <cstdint>
#include <sstream>

#define COLOR_COMPONENT_OVERFLOW_CHECK

/*  TODO:

 * http://viz.aset.psu.edu/gho/sem_notes/color_2d/html/primary_systems.html
 * http://lesscss.org/functions/#color-definition
 * https://en.wikipedia.org/wiki/Alpha_compositing
 * https://en.wikipedia.org/wiki/Blend_modes

 *  Color operator + += Color -> Color::blend
 *      -> operator + (Color const&)
 *  static setBlendMode(Color& Color::*(Color, Color))

 *  Color operator * / float, and with = operator


 *  Add Color Models (HSV, HSL)
 *  https://en.wikipedia.org/wiki/Color_model

 *  ColorComponent:
 *      Add option / set flag to desactivate overflow check on + , - and *

 *  Color Comparision
     bool Color::operator<(const Color& c) const
     {
           return (value() < c.value());
     }

 *  Substract Color ??
 *  https://en.wikipedia.org/wiki/Subtractive_color

 */

class Color final {
private:
    // ARGB
    static constexpr auto ALPHA = 3;
    static constexpr auto RED   = 2;
    static constexpr auto GREEN = 1;
    static constexpr auto BLUE  = 0;

    /**
     *  Sub Classes :
     *      union uColor
     *      class ColorComponent
     */

private:
    /**
     *  union uColor
     */

    union uColor final {
    public:
        uint8_t  component[4];
        uint32_t hex;

    public:
        uColor() { this->hex = 0; }

        uColor(uColor const& c) { *this = c; }

        uColor& operator=(uColor const& c) {
            this->hex = c.hex;
            return *this;
        }

        uColor(uint32_t c) { this->hex = c; }

        uColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
            this->component[RED]   = r;
            this->component[GREEN] = g;
            this->component[BLUE]  = b;
            this->component[ALPHA] = a;
        }
    };

    /**
     *  class ColorComponent
     */

    class ColorComponent final {
    private:
        uint8_t* _component;

    public:
        ColorComponent(uint8_t* component) : _component(component) {}

#ifndef COLOR_COMPONENT_OVERFLOW_CHECK
        operator uint8_t&() const { return *(this->_component); }
#else
        operator uint8_t() const { return *(this->_component); }

        ColorComponent& operator=(uint32_t value) {
            *(this->_component) = value > 0xFF ? 0xFF : value;
            return *this;
        }

        ColorComponent& operator+=(uint8_t value) {
            uint32_t res = *(this->_component) + value;

            *(this->_component) = res > 0xFF ? 0xFF : res;
            return *this;
        }

        ColorComponent& operator-=(uint8_t value) {
            uint32_t res = *(this->_component) - value;

            *(this->_component) = res > 0xFF ? 0x0 : res;
            return *this;
        }

        ColorComponent& operator*=(double value) {
            if (value >= 0) {
                uint32_t res = *(this->_component) * value;

                *(this->_component) = res > 0xFF ? 0xFF : res;
            } else {
                // FIXME
                *(this->_component) = 0;
            }
            return *this;
        }

        ColorComponent& operator/=(double value) {
            if (value >= 0) {
                uint32_t res = *(this->_component) / value;

                *(this->_component) = res > 0xFF ? 0xFF : res;
            } else {
                // FIXME
                *(this->_component) = 0;
            }
            return *this;
        }

        ColorComponent& operator%=(uint8_t value) {
            *(this->_component) %= value;
            return *this;
        }

        ColorComponent& operator&=(uint8_t value) {
            *(this->_component) &= value;
            return *this;
        }

        ColorComponent& operator|=(uint8_t value) {
            *(this->_component) |= value;
            return *this;
        }

        ColorComponent& operator^=(uint8_t value) {
            *(this->_component) ^= value;
            return *this;
        }

        ColorComponent& operator<<=(uint8_t value) {
            *(this->_component) <<= value;
            return *this;
        }

        ColorComponent& operator>>=(uint8_t value) {
            *(this->_component) >>= value;
            return *this;
        }
#endif /* !COLOR_COMPONENT_OVERFLOW_CHECK */
    };

    /**
     *  Class Attributes
     */

private:
    uColor _color;

public:
    ColorComponent r = &(this->_color.component[RED]);
    ColorComponent g = &(this->_color.component[GREEN]);
    ColorComponent b = &(this->_color.component[BLUE]);
    ColorComponent a = &(this->_color.component[ALPHA]);

    /**
     *  C++ Constructor & Cast operator
     */

public:
    Color() {}

    Color(Color const& c) : _color(c._color.hex) { *this = c; }

    Color& operator=(Color const& c) {
        this->_color = c._color;
        return *this;
    }

    Color(uint32_t c) : _color(c) {}

    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : _color(r, g, b, a) {}

    operator uint32_t() const { return this->_color.hex; }

    /**
     *  Color Constructors
     */

public:
    inline static Color rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        return Color(r, g, b, a);
    }

    // 0xAARRGGBB
    inline static Color argb(uint32_t c) { return Color(c); }

    // 0xRRGGBBAA
    static Color rgba(uint32_t c) {
        // (0xRRGGBBAA -> 0xAARRGGBB)
        c = (c & 0x00FFFFFF) << 8 | (c & 0xFF000000) >> 24;
        return Color(c);
    }

    // 0xAABBGGRR
    static Color abgr(uint32_t c) {
        // swap RR and BB (0xAABBGGRR -> 0xAARRGGBB)
        c = c ^ ((c >> 16) & 0x000000FF);
        c = c ^ ((c << 16) & 0x00FF0000);
        c = c ^ ((c >> 16) & 0x000000FF);
        return Color(c);
    }

    // 0xBBGGRRAA
    static Color bgra(uint32_t c) {
        // reverse byte order (0xBBGGRRAA -> 0xAARRGGBB)
        c = (c & 0x0000FFFF) << 16 | (c & 0xFFFF0000) >> 16;
        c = (c & 0x00FF00FF) << 8 | (c & 0xFF00FF00) >> 8;
        return Color(c);
    }

    /**
     *  Color Blend
     *      src: https://wiki.libsdl.org/SDL_SetTextureBlendMode
     */

public:
    class Blend final {
    private:
        inline static uint32_t overflowCheck(uint32_t v) {
            return v > 255 ? 255 : v;
        }

    public:
        static Color none(Color const& c1, Color const& c2) {
            return Color(overflowCheck(c1.r + c2.r),
                         overflowCheck(c1.g + c2.g),
                         overflowCheck(c1.b + c2.b),
                         overflowCheck(c1.a + c2.a));
        }

        static Color alpha(Color const& c1, Color const& c2) {
            float srcA = c2.alpha();

            return Color(overflowCheck(c1.r * (1 - srcA) + c2.r * srcA),
                         overflowCheck(c1.g * (1 - srcA) + c2.g * srcA),
                         overflowCheck(c1.b * (1 - srcA) + c2.b * srcA),
                         overflowCheck(c1.a + c2.a * srcA));
        }

        static Color additive(Color const& c1, Color const& c2) {
            float srcA = c2.alpha();

            return Color(overflowCheck(c1.r + c2.r * srcA),
                         overflowCheck(c1.g + c2.g * srcA),
                         overflowCheck(c1.b + c2.b * srcA),
                         c2.a);
        }

        static Color modulate(Color const& c1, Color const& c2) {
            return Color(overflowCheck(c1.r * c2.r),
                         overflowCheck(c1.g * c2.g),
                         overflowCheck(c1.b * c2.b),
                         c1.a);
        }
    }; /* !Blend */

public:
    inline static void setBlendMode(Color (*mode)(Color const&, Color const&)) {
        Color::_blendMode = mode;
    }

    Color operator+(Color const& c) { return Color::_blendMode(*this, c); }

private:
    static Color (*_blendMode)(Color const&, Color const&);

    /**
     *  Class Utils
     */

public:
    inline float alpha() const { return this->a / 255.f; }

    inline double dist(Color const& c) const {
        return std::sqrt(static_cast<float>(std::pow(c.r - this->r, 2)
                                            + std::pow(c.g - this->g, 2)
                                            + std::pow(c.b - this->b, 2)));
    }

    // ITU-R (International Telecommunication Union recomm.) formula
    inline double value() const {
        return .299 * this->r + .587 * this->g + .114 * this->b;
    }

    std::string toString() const {
        std::ostringstream oss;

        oss << "rgba(r: " << (int) this->r << " g: " << (int) this->g
            << " b: " << (int) this->b << " a: " << (int) this->a << ")";
        return oss.str();
    }
};

#ifndef NO_C_STYLE_COLOR_CONSTRUCTION
inline static Color rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return Color::rgba(r, g, b, a);
}

inline static Color rgb(uint8_t r, uint8_t g, uint8_t b) {
    return rgba(r, g, b, 255);
}
#endif /* !NO_C_STYLE_COLOR_CONSTRUCTION */

Color (*Color::_blendMode)(Color const&, Color const&) = &Color::Blend::none;

#endif /* !COLOR_HPP__ */
