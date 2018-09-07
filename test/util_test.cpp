
#include <gtest/gtest.h>

#include <http/http.h>


TEST(a1, a1) {

	std::string str = "123";

	int i1 = 1;

	http::Parameters pa = { { str,"456"},{ str,http::Parameter::ToString(i1) } };

	int i = 1;
}




