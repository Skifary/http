#include <gtest/gtest.h>

#include <http/http.h>



TEST(ParametersTests, Initializer)
{

	auto params = http::Parameters{ { "username","skifary" },{ "password","123456" },{ "id",0 },{ "float",123.456 },{ "bool", true } };

	EXPECT_EQ(params.format_value_, std::string("username=skifary&password=123456&id=0&float=123.456000&bool=1"));

}
