
#include <gtest/gtest.h>

#include <http/http.h>



TEST(MultipartTests, InitTest)
{

	// from memory
	auto memory_part = http::Part{ "text/html", (http::byte_t*)"html_data" };

	// from filepath
	auto file_part = http::Part{ "c://file.txt" };


	auto multipart = http::Multipart{ { "text/html", (http::byte_t*)"html_data" },http::Part{ "c://file.txt","name" },memory_part,file_part };



}


