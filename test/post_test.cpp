#include <gtest/gtest.h>

#include <http/http.h>

TEST(PostTests, SamplePostTest)
{

	auto resp = http::Post(
		http::URL{ "www.baidu.com" },
		http::Parameters{ { "username","skifary" },{ "password","123456" },{ "id",0 },{ "float",123.456 },{ "bool", true } }
	);

	EXPECT_EQ(1, 1);

}

TEST(PostTests, AsyncTest)
{

	http::PostAsync([](http::Response resp) {

		int i = 1;
	
	}, http::URL{ "www.baidu.com" });


	EXPECT_EQ(1, 1);

}