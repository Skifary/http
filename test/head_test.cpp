#include <gtest/gtest.h>

#include <http/http.h>



TEST(HeadTests, SampleHeadTest)
{

	auto resp = http::Head(
		http::URL{ "www.qq.com" }
	);

}


TEST(HeadTests, AsyncTest)
{

	http::HeadAsync([](http::Response resp) {

		// response

	}, http::URL{ "www.qq.com" });


}
