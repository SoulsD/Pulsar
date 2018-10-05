#include <gtest/gtest.h>
#include "Color.hpp"

namespace {

    class ColorTest : public ::testing::Test {
    protected:
        // You can remove any or all of the following functions if its body
        // is empty.

        ColorTest()
        {
            // You can do set-up work for each test here.
        }

        virtual ~ColorTest()
        {
            // You can do clean-up work that doesn't throw exceptions here.
        }

        // If the constructor and destructor are not enough for setting up
        // and cleaning up each test, you can define the following methods:

        virtual void SetUp()
        {
            // Code here will be called immediately after the constructor (right
            // before each test).
        }

        virtual void TearDown()
        {
            // Code here will be called immediately after each test (right
            // before the destructor).
        }

        // Objects declared here can be used by all tests in the test case for
        // Foo.
    };

}  // namespace

// Tests Color constructors.
TEST_F(ColorTest, DoesConstruct)
{
    {
        Color c;

        ASSERT_EQ((uint32_t) c, 0);

        EXPECT_EQ(0, c.r);
        EXPECT_EQ(0, c.g);
        EXPECT_EQ(0, c.b);
        EXPECT_EQ(0, c.a);
    }

    {
        Color c(255, 255, 255);

        ASSERT_EQ((uint32_t) c, 0xFFFFFFFF);

        EXPECT_EQ(255, c.r);
        EXPECT_EQ(255, c.g);
        EXPECT_EQ(255, c.b);
        EXPECT_EQ(255, c.a);
    }

    {
        Color c(0x778B6C42);  // #8B6C4277
        ASSERT_EQ((uint32_t) c, 0x778B6C42);

        EXPECT_EQ(0x8B, c.r);
        EXPECT_EQ(0x6C, c.g);
        EXPECT_EQ(0x42, c.b);
        EXPECT_EQ(0x77, c.a);
    }

    {
        Color c = 0x8B6C42;  // #8B6C42
        ASSERT_EQ((uint32_t) c, 0xFF8B6C42);

        EXPECT_EQ(0x8B, c.r);
        EXPECT_EQ(0x6C, c.g);
        EXPECT_EQ(0x42, c.b);
        EXPECT_EQ(0xFF, c.a);
        c + (Color) 0x8B6C42;
    }

    {
        auto a = glm::u8vec4(0x8B, 0x6C, 0x42, 0);
        Color c;
        c = a;

        // ASSERT_EQ((uint32_t) c, 0xFF8B6C42);

        EXPECT_EQ(0x8B, c.r);
        EXPECT_EQ(0x6C, c.g);
        EXPECT_EQ(0x42, c.b);
        EXPECT_EQ(0xFF, c.a);
    }

    {
        Color c;

        c = Color::rgb(0x8B6C42);
        EXPECT_EQ((uint32_t) c, 0xFF8B6C42);


        c = Color::rgba(0x8B6C42FF);
        EXPECT_EQ((uint32_t) c, 0xFF8B6C42);

        c = Color::argb(0xFF8B6C42);
        EXPECT_EQ((uint32_t) c, 0xFF8B6C42);
#ifdef COLOR_IMPLICIT_ALPHA_ASSIGNMENT
        c = Color::argb(0x8B6C42);
        EXPECT_EQ((uint32_t) c, 0xFF8B6C42);
#endif

        c = Color::abgr(0xFF426C8B);
        EXPECT_EQ((uint32_t) c, 0xFF8B6C42);

        c = Color::bgra(0x426C8BFF);
        EXPECT_EQ((uint32_t) c, 0xFF8B6C42);
    }

    {
        Color c = rgb(66, 66, 66);  // #424242
        ASSERT_EQ((uint32_t) c, 0xFF424242);
    }

    {
        Color c = rgba(240, 70, 202, .784);  // #F046CA | 200/255 => ~ .784

        ASSERT_EQ((uint32_t) c, 0xC8F046CA);

        EXPECT_EQ(0xF0, c.r);
        EXPECT_EQ(0x46, c.g);
        EXPECT_EQ(0xCA, c.b);
        EXPECT_EQ(0xC8, c.a);

        Color c2(c);

        ASSERT_EQ((uint32_t) c2, 0xC8F046CA);
        ASSERT_EQ((uint32_t) c, (uint32_t) c2);

        EXPECT_EQ(240, c2.r);
        EXPECT_EQ(70, c2.g);
        EXPECT_EQ(202, c2.b);
        EXPECT_EQ(200, c2.a);

        Color c3 = c;

        ASSERT_EQ((uint32_t) c3, 0xC8F046CA);
        ASSERT_EQ((uint32_t) c, (uint32_t) c3);
    }

    {
        EXPECT_EQ(0xFFF046CA, (uint32_t) Color(240, 70, 202, 255));
        EXPECT_EQ(0xFFF046CA, (uint32_t) Color::rgba(240, 70, 202, 1));
        EXPECT_EQ(0xFFF046CA, (uint32_t) rgba(240, 70, 202, 1));
        EXPECT_EQ(0xFFF046CA, (uint32_t) rgba(240, 70, 202, 1.0));
        EXPECT_EQ(0x80F046CA, (uint32_t) rgba(240, 70, 202, .5));
        EXPECT_EQ(0x00F046CA, (uint32_t) rgba(240, 70, 202, .0));
    }
}

// Tests Color comparaison operators.
TEST_F(ColorTest, DoesCompare)
{
    Color const c1 = rgba(240, 240, 240, .90);
    Color c2       = rgb(2, 128, 240);

#ifdef COLOR_INCLUDE_ALPHA_IN_COMPARISON
    EXPECT_TRUE(c1 == rgba(0xf0, 0xf0, 0xf0, .90));
    EXPECT_FALSE(c1 == rgb(0xf0, 0xf0, 0xf0));
#else
    EXPECT_TRUE(c1 == rgba(0xf0, 0xf0, 0xf0, .90));
    EXPECT_TRUE(c1 == rgb(0xf0, 0xf0, 0xf0));
#endif /* !COLOR_INCLUDE_ALPHA_IN_COMPARISON */
    EXPECT_FALSE(c1 == c2);
    EXPECT_TRUE(c1 != c2);
    EXPECT_TRUE(c1 > c2);

    EXPECT_TRUE(c1.b == c2.b);
    EXPECT_TRUE(c1.r != c2.r);
}

// Tests Color assignment.
TEST_F(ColorTest, DoesAssign)
{
    Color c;

    c = 0x8B6C42;
    EXPECT_TRUE((uint32_t) c == (uint32_t) Color(0x8B6C42));
    EXPECT_TRUE(c == Color(0x8B6C42));

    EXPECT_EQ(c.r, 0x8B);
    c.r += 1;
    EXPECT_EQ(c.r, 0x8C);
    EXPECT_EQ((c.r += 1), 0x8D);
    EXPECT_EQ(c.r, 0x8D);
    EXPECT_TRUE((uint32_t) c == (uint32_t) Color(0x8D6C42));
    EXPECT_TRUE(c == Color(0x8D6C42));

#ifdef COLOR_COMPONENT_OVERFLOW_CHECK
    c.r = 0x80;
    EXPECT_EQ((c.r += 0xFF), 0xFF);
    c.r = 0x80;
    EXPECT_EQ((c.r -= 0xFF), 0x00);
    c.r = 0x80;
    EXPECT_EQ((c.r *= 8), 0xFF);
    c.r = 0x80;
    EXPECT_EQ((c.r *= -8), 0x00);
    c.r = 0x80;
    EXPECT_EQ((c.r /= 1000.), 0x00);
    c.r = 0x80;
    EXPECT_EQ((c.r /= .1), 0xFF);
    c.r = 0x80;
    EXPECT_EQ((c.r /= -1000), 0x00);
#endif
}

// Tests Color blending.
TEST_F(ColorTest, DoesBlend)
{
    Color c;

    c = 0x8B6C42;

    Color::setBlendMode(Color::Blend::none);
    Color::setBlendMode(Color::Blend::alpha);
    Color::setBlendMode(Color::Blend::additive);
    Color::setBlendMode(Color::Blend::modulate);

#ifdef COLOR_COMPONENT_OVERFLOW_CHECK
#endif
}

// Tests Color Utils.
TEST_F(ColorTest, Utils)
{
    Color c;

    c = 0x8B6C42;
}
