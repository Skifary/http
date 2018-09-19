#pragma once

/***************************************************************************
*
* Copyright (C) 2018, Skifary, <gskifary@outlook.com>.
*
***************************************************************************/

#include <memory>
#include <functional>
#include <initializer_list>
#include <string>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <vector>
#include <future>

#include <curl/curl.h>

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
	ClassWrapper(DownloadFilePath, std::string)
	ClassWrapper(URL, std::string)
	ClassWrapper(Progress, std::function<void(double)>)
	ClassWrapper(Payload, std::string)
	

	using byte_t = unsigned char;

	struct Field {

		template <typename T>
		Field(std::string key, T value)
			:key_(HTTP_MOVE(key)) {
			std::ostringstream stream;
			stream << value;
			value_ = stream.str();
		};

	public:
		std::string key_;
		std::string value_;
	};

	class Headers
	{
	public:

		Headers() = default;
		Headers(std::string&& header_string);
		Headers(std::string& header_string);
		Headers(std::unordered_map<std::string, std::string>& map);
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
			std::ostringstream stream;
			stream << value;
			_storage_headers[field] = stream.str();

		};

		curl_slist* Chunk();

		Headers& Merge(Headers& other);

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


	// ----------------------------------------------------------------------------------
	//
	//    CURLHandle
	//
	// ----------------------------------------------------------------------------------

	class CURLHandle
	{
	public:
		CURL * curl_;
		curl_slist *chunk_;
		curl_mime *mime_;
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
	//    Multipart
	//
	// ----------------------------------------------------------------------------------

	class Part
	{
	public:

		Part() = default;

		Part(std::string type, byte_t* data, std::string name = "") :type_(HTTP_MOVE(type)), is_file_{ false }, data_((char*)data), name_(name) {};
		Part(std::string filepath, std::string name = "") : is_file_{ true }, data_(filepath), name_(name) {};

	public:

		std::string type_;
		bool is_file_;
		std::string data_;
		std::string name_;
	};

	class Multipart
	{
	public:

		Multipart(const std::initializer_list<Part>& parts);

		curl_mime* Add2Curl(CURL *curl);

	private:
		std::vector<Part> _parts;

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
		void SetOption(DownloadFilePath& filepath);
		void SetOption(Progress& progress);
		void SetOption(Multipart& multipart);
		void SetOption(Payload& payload);

		// method
		Response Get();
		Response Post();
		Response Head();

	private:

		// options
		void __set_url(URL& url);
		void __set_parameters(Parameters& parameters);
		void __set_headers(Headers& headers);
		void __set_download_filepath(DownloadFilePath& filepath);
		void __set_progress(Progress& progress);
		void __set_multipart(Multipart& multipart);
		void __set_payload(Payload& payload);

		// core request
		Response __request(CURL *curl);

		// curl life manager
		CURLHandle* __curl_handle_init();
		static void __curl_handle_free(CURLHandle* handle);

		// resp custom deleter
		static void __resp_data_deleter(struct __write_data_t *ptr);

		// curl write callback
		static size_t __write_function(void* ptr, size_t size, size_t nmemb, struct __write_data_t *data);
		// curl progress callback
		static int __xfer_info(void *data, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

	private:

		URL _url;

		Parameters _parameters;

		Progress _progress;

		std::unique_ptr<CURLHandle, std::function<void(CURLHandle *)>> _curl_handle_ptr;
		std::shared_ptr<struct __write_data_t> _response_data_ptr;
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
			__set_option(session, HTTP_FWD(ts)...);
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

	// Get Async
	template <typename... Ts>
	std::future<void> GetAsync(std::function<void(Response)> complete, Ts... ts) {
		return std::async(std::launch::async,
			[complete](Ts... ts) {
			auto resp = Get(HTTP_MOVE(ts)...);
			complete(HTTP_MOVE(resp));
		},
			HTTP_MOVE(ts)...);
	}

	// Post 
	template <typename... Ts>
	Response Post(Ts&&... ts) {
		Session session;
		priv::__set_option(session, HTTP_FWD(ts)...);
		return session.Post();
	}

	// Post Async
	template <typename... Ts>
	std::future<void> PostAsync(std::function<void(Response)> complete, Ts... ts) {
		return std::async(std::launch::async,
			[complete](Ts... ts) {
			auto resp = Post(HTTP_MOVE(ts)...);
			complete(HTTP_MOVE(resp));
		},
			HTTP_MOVE(ts)...);
	}

	// Head 
	template <typename... Ts>
	Response Head(Ts&&... ts) {
		Session session;
		priv::__set_option(session, HTTP_FWD(ts)...);
		return session.Post();
	}

	// Head Async
	template <typename... Ts>
	std::future<void> HeadAsync(std::function<void(Response)> complete, Ts... ts) {
		return std::async(std::launch::async,
			[complete](Ts... ts) {
			auto resp = Head(HTTP_MOVE(ts)...);
			complete(HTTP_MOVE(resp));
		},
			HTTP_MOVE(ts)...);
	}

	
} // namespace http


