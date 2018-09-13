#include <gtest/gtest.h>

#include <http/http.h>



TEST(GetTests, SampleGetTest)
{

	auto resp = http::Get(
		http::URL{ "www.baidu.com" }
	);


	EXPECT_EQ(1, 1);

}

TEST(GetTests, DownloadTest)
{

	http::Get(
		http::URL{ "www.baidu.com" },
		http::DownloadFilePath{"C:\\Users\\guotianyuan\\Desktop\\baidu.html"}
	);

}


