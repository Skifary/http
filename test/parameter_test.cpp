#include <gtest/gtest.h>

#include <http/http.h>



TEST(ParametersTests, Initializer)
{

	auto params = http::Parameters{ { "username","skifary" },{ "password","123456" },{ "id",0 },{ "float",123.456 },{ "bool", true } };




}
