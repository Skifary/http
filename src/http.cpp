
#include <http/http.h>



namespace http {

	namespace priv {

		namespace util {

			std::string __url_encode(const std::string& value_to_escape);

		}

	}

	enum __write_data_type
	{
		stream,
		string,
	};

	struct __write_data_t
	{
		__write_data_type type_;
		std::string string_data_;
		std::ofstream stream_data_;

		__write_data_t() { type_ = __write_data_type::string; };
		__write_data_t(__write_data_type type) { type_ = type; };

		void SetData(void* ptr, size_t size) {

			switch (type_)
			{
			case http::stream:
				stream_data_.write(static_cast<char*>(ptr), size);
				break;
			case http::string:
				string_data_.append(static_cast<char*>(ptr), size);
				break;
			default:
				break;
			}
		};

		std::string TryGetStringData() {
			if (type_ == string)
			{
				return string_data_;
			}

			return "";
		};

		void OpenStream(std::string& filepath) {
			stream_data_.open(filepath, std::ios::trunc | std::ios::binary);
		};

		void TryCloseStream() {
			if (type_ == stream && stream_data_.is_open())
			{
				stream_data_.close();
			}
		};
	};


	// ----------------------------------------------------------------------------------
	//
	//    Parameter
	//
	// ----------------------------------------------------------------------------------

	Parameters::Parameters(const std::initializer_list<Parameter>& parameters)
	{
		for (const auto& param : parameters)
		{
			AddParameter(param);
		}
	}

	void Parameters::AddParameter(const Parameter& parameter)
	{
		if (!format_value_.empty())
		{
			format_value_ += "&";
		}

		auto escaped_key = priv::util::__url_encode(parameter.key_);
		if (parameter.value_.empty())
		{
			format_value_ += escaped_key;
		}
		else
		{
			auto escaped_value = priv::util::__url_encode(parameter.value_);
			format_value_ += escaped_key + "=" + escaped_value;
		}
	}


	// ----------------------------------------------------------------------------------
	//
	//    Session
	//
	// ----------------------------------------------------------------------------------

	Session::Session()
	{
		_curl_handle_ptr = std::unique_ptr<CURLHandle, std::function<void(CURLHandle *)>>(Session::__curl_handle_init(), &Session::__curl_handle_free);

		// don't use make_unique, because it's completed in c++14
		//_response_data_ptr = std::make_unique<struct __write_data_t>();
		// unique_ptr can't use an incomplete type
		// so i change it to shared_ptr
		_response_data_ptr = std::make_shared<struct __write_data_t>();

		auto curl = _curl_handle_ptr->curl_;

		if (curl)
		{
			//curl init 

		}

	}

	// public
	Response Session::Get()
	{
		return __request(_curl_handle_ptr->curl_);
	}

	// options
	void Session::SetOption(URL& url) { __set_url(url); }
	void Session::SetOption(Parameters& parameters) { __set_parameters(parameters); };
	void Session::SetOption(Headers& headers) { __set_headers(headers); };

	// private
	void Session::__set_url(URL& url) { _url = url; }
	void Session::__set_parameters(Parameters& parameters) { _parameters = parameters; }
	void Session::__set_headers(Headers& headers) { _headers = headers; }

	Response Session::__request(CURL *curl)
	{
		CURLcode res = CURLE_OK;

		// set url
		auto url = _url.value_ + "?" + _parameters.format_value_;
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		__write_data_t head_string;

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &__write_function);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, _response_data_ptr.get());
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &head_string);

		res = curl_easy_perform(curl);

		long resp_code;
		std::string error;

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);

		if (res != CURLE_OK)
		{
			error = curl_easy_strerror(res);
		}

		return Response(
			HTTP_MOVE(resp_code), 
			HTTP_MOVE(_response_data_ptr->TryGetStringData()),
			HTTP_MOVE(Headers(head_string.TryGetStringData())), 
			HTTP_MOVE(error)
		);

	}

	// curl init
	CURLHandle* Session::__curl_handle_init()
	{
		auto handle = new CURLHandle();
		handle->curl_ = curl_easy_init();
		return handle;
	}

	// curl free
	void Session::__curl_handle_free(CURLHandle* handle)
	{
		curl_easy_cleanup(handle->curl_);
	}

	// write function call back

	size_t Session::__write_function(void* ptr, size_t size, size_t nmemb, __write_data_t *data)
	{
		size_t append_size = size * nmemb;
		data->SetData(ptr, append_size);
		return append_size;
	}


	// ----------------------------------------------------------------------------------
	//
	//    private util
	//
	// ----------------------------------------------------------------------------------

	std::string priv::util::__url_encode(const std::string& value_to_escape)
	{
		std::string escaped;
		char *escape_control = curl_easy_escape(nullptr, value_to_escape.c_str(), value_to_escape.size());
		escaped = escape_control;
		curl_free(escape_control);
		return escaped;
	}


	// ----------------------------------------------------------------------------------
	//
	//    Headers 
	//
	// ----------------------------------------------------------------------------------

	Headers::Headers(std::string&& header_string)
	{

	}

	// ----------------------------------------------------------------------------------
	//
	//    Response 
	//
	// ----------------------------------------------------------------------------------

	Response::Response(int&& code, std::string&& response_string, Headers&& headers, std::string&& error)
		: code_(code), response_string_(response_string), headers_(headers), error_(error) {}

}