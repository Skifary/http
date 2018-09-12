#pragma once


#include <memory>
#include <functional>
#include <initializer_list>
#include <string>
#include <curl/curl.h>

#include <fstream>
#include <unordered_map>
#include <sstream>

#include <algorithm>



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

	// declare wrapper class
	ClassWrapper(FilePath, std::string)
	ClassWrapper(URL, std::string)

	// declare
	class Session;

	struct Field {

		template <typename T>
		Field(std::string key, T value)
			:key_(HTTP_MOVE(key)) {
			value_ = HTTP_MOVE(std::to_string(value));
		};

		// Field template specialization for std::string and const char*
		template <>
		Field(std::string key, std::string value)
			:key_(HTTP_MOVE(key)), value_(HTTP_MOVE(value)) {};
		template <>
		Field(std::string key, const char* value)
			:key_(HTTP_MOVE(key)), value_(HTTP_MOVE(std::string(value))) {};

	public:
		std::string key_;
		std::string value_;
	};

	class Headers
	{
	public:
		Headers() = default;

		Headers(std::string& header_string);

		Headers(const std::initializer_list<Field>& headers);

		// field

		template <typename T>
		T GetField(std::string field) {
			
			// avoid inserting a null value
			T t{};
			auto itr = _storage_headers.find(field);
			if (itr == _storage_headers.end())
			{
				return t;
			}

			std::istringstream stream(itr->second);
			stream >> t;
			return HTTP_MOVE(t);
		};

		// GetField template specialization for std::string
		template <>
		std::string GetField(std::string field) {

			// avoid inserting a null value
			auto itr = _storage_headers.find(field);
			if (itr == _storage_headers.end())
			{
				return "";
			}

			return _storage_headers[field];
		};

		template <>
		bool GetField(std::string field) {

			// avoid inserting a null value
			auto itr = _storage_headers.find(field);
			if (itr == _storage_headers.end())
			{
				return false;
			}

			// compatible both "0&&1" and "true&&false"
			if (_storage_headers[field] == "true")
			{
				return true;
			}

			bool b;
			std::istringstream stream(itr->second);
			stream >> b;
			return HTTP_MOVE(b);
		};

		template <typename T>
		void SetField(std::string field, T value) {
			_storage_headers[field] = HTTP_MOVE(std::to_string(value));
		};

		// SetField template specialization for std::string and const char*
		template <>
		void SetField<std::string>(std::string field, std::string value) {
			_storage_headers[field] = HTTP_MOVE(value);
		};
		template <>
		void SetField<const char *>(std::string field, const char *value) {
			_storage_headers[field] = HTTP_MOVE(std::string(value));
		};

		curl_slist* Chunk();

	private:

		void __parse_http_header(std::string& header_string);

	private:

		std::unordered_map<std::string, std::string> _storage_headers;

	};

	// response
	class Response
	{
	public:
		Response() = default;
		Response(int&& code, std::string&& body, Headers&& headers, std::string&& error);

	public:

		int code_;

		std::string body_;
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

		curl_slist *chunk_;

	};

	// ----------------------------------------------------------------------------------
	//
	//    Parameter
	//
	// ----------------------------------------------------------------------------------

	class Parameters
	{
	public:

		Parameters() = default;
		Parameters(const std::initializer_list<Field>& parameters);

		void AddParameter(const Field& parameter);

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

		std::unique_ptr<CURLHandle, std::function<void(CURLHandle *)>> _curl_handle_ptr;
		std::shared_ptr<struct __write_data_t> _response_data_ptr;
	};

} // namespace http


