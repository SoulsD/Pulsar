#ifndef COLOR_HPP__
#define COLOR_HPP__

#include <cmath>
#include <cstdint>
#include <sstream>

#include "Constants.hh"

/*  TODO:

 *  http://viz.aset.psu.edu/gho/sem_notes/color_2d/html/primary_systems.html
 *  http://lesscss.org/functions/#color-definition
 *  https://en.wikipedia.org/wiki/Alpha_compositing

 *  https://en.wikipedia.org/wiki/Blend_modes

 *  https://en.wikipedia.org/wiki/CIELAB_color_space#CIELAB

 *  https://github.com/halostatue/color

 *  Blend default via #define

 *  Color operator * / float, and with = operator

 *  getter argb / rgba ... | uint8, uint16, ...
 *      => "#AARRGGBB", "#RRGGBBAA"
 *  Add Color Models (HSV, HSL)
 *  https://en.wikipedia.org/wiki/Color_model

 *  ColorComponent:
 *      Add option / set flag to desactivate overflow check on + , - and *

 *  User-defined literals ->     "#RRGGBBAA"_rgba;
 *   Color operator""_rgb(const char*, std::size_t) {
         return Color();
     }

 *  Substract Color ??
 *  https://en.wikipedia.org/wiki/Subtractive_color

 */

using std::uint32_t;
using std::uint8_t;

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
     *  Union uColor
     */

    union uColor final {
    public:
        uint8_t component[4];
        uint32_t hex;

    public:
        uColor() { this->hex = 0; }

        uColor(uColor const& c) { *this = c; }

        uColor& operator=(uColor const& c)
        {
            this->hex = c.hex;
            return *this;
        }

        uColor& operator=(uColor&&) = default;

        uColor(uint32_t c)
        {  // /!\ 0xAARRGGBB
            this->hex = c;
        }

        uColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        {
            this->component[RED]   = r;
            this->component[GREEN] = g;
            this->component[BLUE]  = b;
            this->component[ALPHA] = a;
        }
    }; /* !uColor */

    /**
     *  Class ColorComponent
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

        ColorComponent& operator=(uint32_t value)
        {
            *(this->_component) = std::min(value, 0xFFu);
            return *this;
        }

        ColorComponent& operator+=(uint8_t value)
        {
            uint32_t res = *(this->_component) + value;

            *(this->_component) = std::min(res, 0xFFu);
            return *this;
        }

        ColorComponent& operator-=(uint8_t value)
        {
            uint8_t res = *(this->_component) - value;

            *(this->_component) = value >= *(this->_component) ? 0x0 : res;
            return *this;
        }

        ColorComponent& operator*=(double value)
        {
            if (value > 0) {
                uint32_t res = std::round(*(this->_component) * value);

                *(this->_component) = std::min(res, 0xFFu);
            }
            else {
                *(this->_component) = 0;
            }
            return *this;
        }

        ColorComponent& operator/=(double value)
        {
            uint32_t res;

            if (value >= 1) {
                res = std::round(*(this->_component) / value);

                *(this->_component) = std::max(res, 0x00u);
            }
            else if (value > 0) {
                res = std::round(*(this->_component) / value);

                *(this->_component) = std::min(res, 0xFFu);
            }
            else {
                *(this->_component) = 0;
            }
            return *this;
        }

        ColorComponent& operator%=(uint8_t value)
        {
            *(this->_component) %= value;
            return *this;
        }

        ColorComponent& operator&=(uint8_t value)
        {
            *(this->_component) &= value;
            return *this;
        }

        ColorComponent& operator|=(uint8_t value)
        {
            *(this->_component) |= value;
            return *this;
        }

        ColorComponent& operator^=(uint8_t value)
        {
            *(this->_component) ^= value;
            return *this;
        }

        ColorComponent& operator<<=(uint8_t value)
        {
            *(this->_component) <<= value;
            return *this;
        }

        ColorComponent& operator>>=(uint8_t value)
        {
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
     *  C++ Constructor
     */

public:
    Color() {}

    Color(Color const& c) { *this = c; }

    Color& operator=(Color const& c) = default;
    // {
    //     this->_color = c._color;
    //     return *this;
    // }

    Color(Color&& c) : _color(c._color) {}  // = default;
    Color& operator=(Color&& c)
    {
        this->_color = c._color;
        return *this;
    }  // = default;

    Color(uint32_t c) : _color(c)
    {
#ifdef COLOR_IMPLICIT_ALPHA_ASSIGNMENT
        if (this->a == 0) {
            this->a = 255;
        }
#endif /* !COLOR_IMPLICIT_ALPHA_ASSIGNMENT */
    }

#ifdef COLOR_IMPLICIT_ALPHA_ASSIGNMENT
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : _color(r, g, b, a) {}
#else
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0) : _color(r, g, b, a) {}
#endif

    /**
     *  Color Constructors
     */

public:
    inline static Color rgb(uint8_t r, uint8_t g, uint8_t b) { return Color(r, g, b); }

    inline static Color rgba(uint8_t r, uint8_t g, uint8_t b, double a = .0)
    {
        uint8_t alpha
            = std::min(std::max(static_cast<int>(std::round(0xFF * a)), 0x00), 0xFF);
        return Color(r, g, b, alpha);
    }

    // 0xRRGGBB
    inline static Color rgb(uint32_t c)
    {
        // force Alpha to 00
        c = (c & ~0xFF000000);
        return Color(c);
    }

    // 0xAARRGGBB
    inline static Color argb(uint32_t c) { return Color(c); }

    // 0xRRGGBBAA
    inline static Color rgba(uint32_t c)
    {
        // (0xRRGGBBAA -> 0xAARRGGBB)
        c = (c & 0xFFFFFF00) >> 8 | (c & 0x000000FF) << 24;
        return Color(c);
    }

    // 0xAABBGGRR
    inline static Color abgr(uint32_t c)
    {
        // swap RR and BB (0xAABBGGRR -> 0xAARRGGBB)
        c = c ^ ((c >> 16) & 0x000000FF);
        c = c ^ ((c << 16) & 0x00FF0000);
        c = c ^ ((c >> 16) & 0x000000FF);
        return Color(c);
    }

    // 0xBBGGRRAA
    inline static Color bgra(uint32_t c)
    {
        // reverse byte order (0xBBGGRRAA -> 0xAARRGGBB)
        c = (c & 0x0000FFFF) << 16 | (c & 0xFFFF0000) >> 16;
        c = (c & 0x00FF00FF) << 8 | (c & 0xFF00FF00) >> 8;
        return Color(c);
    }


    /**
     *  Color Assignment overload
     */

    operator uint32_t() const { return this->_color.hex; }

    Color& operator=(uint32_t c)
    {
        this->_color = c;
#ifdef COLOR_IMPLICIT_ALPHA_ASSIGNMENT
        if (this->a == 0) {
            this->a = 255;
        }
#endif /* !COLOR_IMPLICIT_ALPHA_ASSIGNMENT */
        return *this;
    }

    /**
     *  Relational Operators
     *      Implicit comparison with integers disabled
     *
     *  TODO:
     *      https://stackoverflow.com/a/9019461
     *      https://en.wikipedia.org/wiki/Color_difference
     */

    inline bool operator==(Color const& c) const
    {
#ifdef COLOR_INCLUDE_ALPHA_IN_COMPARISON
        return this->_color.hex == c._color.hex;
#else
        return this->_color.component[RED] == c._color.component[RED]
               && this->_color.component[GREEN] == c._color.component[GREEN]
               && this->_color.component[BLUE] == c._color.component[BLUE];
#endif /* !COLOR_INCLUDE_ALPHA_IN_COMPARISON */
    }
    inline bool operator!=(Color const& c) const { return !(*this == c); }

    inline bool operator<(Color const& c) const { return this->value() < c.value(); }
    inline bool operator>(Color const& c) const { return c < *this; }
    inline bool operator<=(Color const& c) const { return !(*this > c); }
    inline bool operator>=(Color const& c) const { return !(*this < c); }

    /**
     *  Explicitly Deleted Operators
     */

    Color& operator++()        = delete;
    Color operator++(int)      = delete;
    Color& operator--()        = delete;
    Color operator--(int)      = delete;
    Color operator+(int) const = delete;
    Color operator-(int) const = delete;
    Color operator*(int) const = delete;
    Color operator/(int) const = delete;
    Color operator%(int) const = delete;
    Color operator+=(int)      = delete;
    Color operator-=(int)      = delete;
    Color operator*=(int)      = delete;
    Color operator/=(int)      = delete;
    Color operator%=(int)      = delete;

    /**
     *  Color Blend
     *      src: https://wiki.libsdl.org/SDL_SetTextureBlendMode
     */

public:
    typedef Color (*BlendMode_t)(Color const&, Color const&);

    class Blend final {
    private:
        inline static uint32_t overflowCheck(uint32_t v)
        {
            // v1, v2 cast from uint8_t to uint32_t
#ifdef COLOR_COMPONENT_OVERFLOW_CHECK
            return std::min(v, 0xFFu);
#else
            return v;
#endif
        }

    public:
        static Color none(Color const& c1, Color const& c2)
        {
            return Color(overflowCheck(c1.r + c2.r),
                         overflowCheck(c1.g + c2.g),
                         overflowCheck(c1.b + c2.b),
                         overflowCheck(c1.a + c2.a));
        }

        static Color alpha(Color const& c1, Color const& c2)
        {
            float srcA = c2.alpha();

            return Color(overflowCheck(c1.r * (1 - srcA) + c2.r * srcA),
                         overflowCheck(c1.g * (1 - srcA) + c2.g * srcA),
                         overflowCheck(c1.b * (1 - srcA) + c2.b * srcA),
                         overflowCheck(c1.a + c2.a * srcA));
        }

        static Color additive(Color const& c1, Color const& c2)
        {
            float srcA = c2.alpha();

            return Color(overflowCheck(c1.r + c2.r * srcA),
                         overflowCheck(c1.g + c2.g * srcA),
                         overflowCheck(c1.b + c2.b * srcA),
                         c2.a);
        }

        static Color modulate(Color const& c1, Color const& c2)
        {
            return Color(overflowCheck(c1.r * c2.r),
                         overflowCheck(c1.g * c2.g),
                         overflowCheck(c1.b * c2.b),
                         c1.a);
        }
    }; /* !Blend */

public:
    inline static void setBlendMode(BlendMode_t mode) { Color::_blendMode = mode; }

    inline static BlendMode_t getBlendMode() { return Color::_blendMode; }

    Color& operator+=(Color const& c) { return (*this = Color::_blendMode(*this, c)); }

    // friends defined inside class body are inline and are hidden from non-ADL
    // lookup
    friend Color operator+(Color lhs, Color const& rhs)
    {
        lhs += rhs;  // reuse compound assignment
        return lhs;  // return the result by value (uses move constructor)
    }

private:
    static BlendMode_t _blendMode;


    /**
     *  Class Utils
     */

public:
    inline double alpha() const { return this->a / 255.; }

    inline double dist(Color const& c) const
    {
        return std::sqrt(static_cast<double>(std::pow(c.r - this->r, 2)
                                             + std::pow(c.g - this->g, 2)
                                             + std::pow(c.b - this->b, 2)));
    }

    // ITU-R (International Telecommunication Union recomm.) formula
    inline double value() const
    {
        return .299 * this->r + .587 * this->g + .114 * this->b;
    }

    std::string toString() const
    {
        std::ostringstream oss;

        oss << "rgba(r: " << (int) this->r << " g: " << (int) this->g
            << " b: " << (int) this->b << " a: " << (int) this->a << ")";
        return oss.str();
    }
}; /* !Color */

std::ostream& operator<<(std::ostream& os, Color const& color)
{
    os << color.toString();
    return os;
}

/**
 *  Disable implicit comparison with integers
 */

inline bool operator==(Color const&, uint32_t const&) = delete;
inline bool operator==(uint32_t const&, Color const&) = delete;
inline bool operator!=(Color const&, uint32_t const&) = delete;
inline bool operator!=(uint32_t const&, Color const&) = delete;

inline bool operator<(Color const&, uint32_t const&)  = delete;
inline bool operator<(uint32_t const&, Color const&)  = delete;
inline bool operator>(Color const&, uint32_t const&)  = delete;
inline bool operator>(uint32_t const&, Color const&)  = delete;
inline bool operator<=(Color const&, uint32_t const&) = delete;
inline bool operator<=(uint32_t const&, Color const&) = delete;
inline bool operator>=(Color const&, uint32_t const&) = delete;
inline bool operator>=(uint32_t const&, Color const&) = delete;

/**
 *  C style Color construction
 */

#ifndef COLOR_NO_C_STYLE_CONSTRUCTION
inline static Color rgba(uint8_t r, uint8_t g, uint8_t b, double a = .0)
{
    return Color::rgba(r, g, b, a);
}

inline static Color rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return Color::rgb(r, g, b);
}
#endif /* !COLOR_NO_C_STYLE_CONSTRUCTION */

/**
 *  Blend mode default value
 */

Color (*Color::_blendMode)(Color const&, Color const&) = &Color::Blend::none;

#endif /* !COLOR_HPP__ */
