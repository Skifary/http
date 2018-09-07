#pragma once

// new header

#include <memory>
#include <functional>
#include <initializer_list>
#include <string>
#include <curl/curl.h>

#include <fstream>
#include <unordered_map>

namespace http {


	// http status code

#define HTTP_OK 200

	
	// common 
#define HTTP_FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)
#define HTTP_MOVE(...) ::std::move(__VA_ARGS__)

#define HTTP_ASSERT(expr) assert((expr))
#define HTTP_STATIC_ASSERT(expr, msg) static_assert(expr,msg)

	// class wrapper
#define ClassWrapper(cls, type) class cls\
	{\
	public:\
		cls() = default;\
		cls(type& value):value_(value){};\
		cls(type&& value):value_(HTTP_MOVE(value)){};\
	public:\
		type value_;\
	};


	// define wrapper class

	ClassWrapper(FilePath, std::string)
	ClassWrapper(URL, std::string)

	// define
	class Session;

	class Headers
	{
	public:
		Headers() = default;

		Headers(std::string&& header_string);

	private:

	};

	// response
	class Response
	{
	public:
		Response() = default;
		Response(int&& code, std::string&& response_string, Headers&& headers, std::string&& error);

	public:

		int code_;

		std::string response_string_;
		Headers headers_;

		std::string error_;

	};

	// private
	namespace priv {

		template <typename T>
		void __set_option(Session& session, T&& t)
		{
			session.SetOption(HTTP_FWD(t));
		}

		template <typename T, typename... Ts>
		void __set_option(Session& session, T&& t, Ts&&... ts)
		{
			__set_option(session, HTTP_FWD(t));
			__set_option(session, HTTP_FWD(ts...));
		}

	} // namespace priv


	// ----------------------------------------------------------------------------------
	//
	//    public api
	//
	// ----------------------------------------------------------------------------------

	// Get 
	template <typename... Ts>
	Response Get(Ts&&... ts) {
		Session session;
		priv::__set_option(session, HTTP_FWD(ts)...);
		return session.Get();
	}


	// ----------------------------------------------------------------------------------
	//
	//    CURLHandle
	//
	// ----------------------------------------------------------------------------------

	class CURLHandle
	{
	public:
		CURL *curl_;

	};

	// ----------------------------------------------------------------------------------
	//
	//    Parameter
	//
	// ----------------------------------------------------------------------------------

	struct Parameter {

		template <typename Key, typename Value>
		Parameter(Key&& key, Value&& value)
			:key_(HTTP_FWD(key)), value_(HTTP_FWD(value)) {};

		template <typename T>
		inline static std::string ToString(T&& t)
		{
			return std::to_string(t);
		};

	public:
		std::string key_;
		std::string value_;
	};

	class Parameters
	{
	public:

		Parameters() = default;
		Parameters(const std::initializer_list<Parameter>& parameters);

		void AddParameter(const Parameter& parameter);

	public:
		std::string format_value_;
	};


	// ----------------------------------------------------------------------------------
	//
	//    Session
	//
	// ----------------------------------------------------------------------------------
	class Session
	{
	public:

		Session();

		// options
		void SetOption(URL& url);
		void SetOption(Parameters& parameters);
		void SetOption(Headers& headers);

		// method
		Response Get();

	private:

		// options
		void __set_url(URL& url);
		void __set_parameters(Parameters& parameters);
		void __set_headers(Headers& headers);

		// core request
		Response __request(CURL *curl);

		// curl life manager
		CURLHandle* __curl_handle_init();
		static void __curl_handle_free(CURLHandle* handle);

		// curl write call back
		static size_t __write_function(void* ptr, size_t size, size_t nmemb, struct __write_data_t *data);

	private:

		URL _url;

		Parameters _parameters;
		Headers _headers;

		std::unique_ptr<CURLHandle, std::function<void(CURLHandle *)>> _curl_handle_ptr;
		std::shared_ptr<struct __write_data_t> _response_data_ptr;
	};

} // namespace http


