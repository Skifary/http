#include <gtest/gtest.h>

#include <http/http.h>

TEST(HeadersTests, SetterGetter)
{
	http::Headers h;

	// setter
	h.SetField("str_value", std::string("str_value"));
	h.SetField("const_char_ptr_value", "const_char_ptr_value");
	h.SetField("int_value", 100);
	h.SetField("double_value", 567.5);
	h.SetField("bool_value", true);

	//getter
	EXPECT_EQ(100, h.GetField<int>("int_value"));
	EXPECT_DOUBLE_EQ(567.5, h.GetField<double>("double_value"));
	EXPECT_EQ(std::string("const_char_ptr_value"), h.GetField<std::string>("const_char_ptr_value"));
	EXPECT_EQ(std::string("str_value"), h.GetField<std::string>("str_value"));
	EXPECT_EQ(true, h.GetField<bool>("bool_value"));

}


TEST(HeadersTests, ResponseBodyParse)
{
	auto header_string = std::string{
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx\r\n"
		"Date: Sun, 05 Mar 2017 00:34:54 GMT\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: 351\r\n"
		"Connection: keep-alive\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Access-Control-Allow-Credentials: true\r\n"
		"\r\n" };

	http::Headers h(header_string);

	EXPECT_EQ(std::string("nginx"), h.GetField<std::string>("Server"));
	EXPECT_EQ(std::string("Sun, 05 Mar 2017 00:34:54 GMT"), h.GetField<std::string>("Date"));
	EXPECT_EQ(std::string("application/json"), h.GetField<std::string>("Content-Type"));
	EXPECT_EQ(351, h.GetField<int>("Content-Length"));
	EXPECT_EQ(std::string("keep-alive"), h.GetField<std::string>("Connection"));
	EXPECT_EQ(std::string("*"), h.GetField<std::string>("Access-Control-Allow-Origin"));
	EXPECT_EQ(true, h.GetField<bool>("Access-Control-Allow-Credentials"));

}


TEST(HeadersTests, InitializerList)
{

	auto h = http::Headers{ { "username","skifary" },{ "password","123456" },{ "id",376512563 },{ "float",123.456 },{ "bool", true } };;

	EXPECT_EQ(std::string("skifary"), h.GetField<std::string>("username"));
	EXPECT_EQ(std::string("123456"), h.GetField<std::string>("password"));
	EXPECT_EQ(376512563, h.GetField<int>("id"));
	EXPECT_FLOAT_EQ(123.456, h.GetField<float>("float"));
	EXPECT_EQ(true, h.GetField<bool>("bool"));

}
