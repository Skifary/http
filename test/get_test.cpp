#include <gtest/gtest.h>

#include <http/http.h>



TEST(GetTests, SampleGetTest)
{

	auto resp = Get(http::URL("www.baidu.com"));

	EXPECT_EQ(1, 1);

}
