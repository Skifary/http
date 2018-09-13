#include <gtest/gtest.h>

#include <http/http.h>


TEST(ProgressTests, GetProgress)
{
	auto resp = http::Get(
		http::URL{ "www.baidu.com" },
		http::Progress{ [](double progress) {
		std::cout << progress << std::endl;
	} }
	);

	EXPECT_EQ(1, 1);
}