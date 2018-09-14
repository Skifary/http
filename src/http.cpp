
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

	Parameters::Parameters(const std::initializer_list<Field>& parameters)
	{
		for (const auto& param : parameters)
		{
			AddParameter(param);
		}
	}

	void Parameters::AddParameter(const Field& parameter)
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
		//_response_data_ptr = std::make_shared<struct __write_data_t>();
		// use make_shared can't add custom deleter
		// use shared_ptr to pass custom deleter
		_response_data_ptr = std::shared_ptr<struct __write_data_t>(new __write_data_t, __resp_data_deleter);
		auto curl = _curl_handle_ptr->curl_;

		if (curl)
		{
			//curl defaults 
			auto version_info = curl_version_info(CURLVERSION_NOW);
			auto version = std::string{ "curl/" } + std::string{ version_info->version };
			curl_easy_setopt(curl, CURLOPT_USERAGENT, version.data());
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
			curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);

		}

	}

	// public
	Response Session::Get()
	{
		auto curl = _curl_handle_ptr->curl_;
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
		}

		return __request(curl);
	}

	Response Session::Post()
	{
		auto curl = _curl_handle_ptr->curl_;
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

			auto payload = HTTP_MOVE(_parameters.format_value_);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
		}

		return __request(curl);
	}

	Response Session::Head()
	{
		auto curl = _curl_handle_ptr->curl_;
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, NULL);
			curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
		}
		return __request(curl);
	}

	// options
	void Session::SetOption(URL& url) { __set_url(url); }
	void Session::SetOption(Parameters& parameters) { __set_parameters(parameters); }
	void Session::SetOption(Headers& headers) { __set_headers(headers); }
	void Session::SetOption(DownloadFilePath& filepath) { __set_download_filepath(filepath); }
	void Session::SetOption(Progress& progress) { __set_progress(progress); }
	void Session::SetOption(Multipart& multipart) { __set_multipart(multipart); };

	// private
	void Session::__set_url(URL& url) { _url = url; }
	void Session::__set_parameters(Parameters& parameters) { _parameters = parameters; }

	void Session::__set_headers(Headers& headers) 
	{ 
		auto curl = _curl_handle_ptr->curl_;

		if (curl)
		{
			auto chunk = headers.Chunk();
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
			curl_slist_free_all(_curl_handle_ptr->chunk_);
			_curl_handle_ptr->chunk_ = chunk;
		}
	}

	void Session::__set_download_filepath(DownloadFilePath& filepath)
	{
		_response_data_ptr->type_ = stream;
		_response_data_ptr->OpenStream(filepath.value_);
	}

	void Session::__set_progress(Progress& progress)
	{
		auto curl = _curl_handle_ptr->curl_;

		if (curl)
		{
			_progress = progress;
			curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &__xfer_info);
			curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		}
	}

	void Session::__set_multipart(Multipart& multipart)
	{
		auto curl = _curl_handle_ptr->curl_;

		if (curl)
		{
			auto mime = multipart.Add2Curl(curl);
			curl_mime_free(_curl_handle_ptr->mime_);
			_curl_handle_ptr->mime_ = mime;
		}
	}

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

		long resp_code = -1;
		std::string error;

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);

		if (res != CURLE_OK)
		{
			error = curl_easy_strerror(res);
			std::cout << "[curl error] : " << std::endl << "[code] " << res << std::endl << "[message] " << error << std::endl;
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
		handle->chunk_ = nullptr;
		handle->mime_ = nullptr;
		return handle;
	}

	// curl free
	void Session::__curl_handle_free(CURLHandle* handle)
	{
		curl_easy_cleanup(handle->curl_);
		curl_slist_free_all(handle->chunk_);
		curl_mime_free(handle->mime_);
		delete handle;
	}

	void Session::__resp_data_deleter(__write_data_t *ptr)
	{
		ptr->TryCloseStream();
		delete ptr;
	}

	// write function callback
	size_t Session::__write_function(void* ptr, size_t size, size_t nmemb, __write_data_t *data)
	{
		size_t append_size = size * nmemb;
		data->SetData(ptr, append_size);
		return append_size;
	}

	// progress callback
	int Session::__xfer_info(void *data, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
	{
		auto session = (Session *)data;
		if (session->_progress.value_)
		{
			double progress = 0.0;

			if (dltotal)
			{
				progress = (double)dlnow / (double)dltotal;
			}
			else if (ultotal)
			{
				progress = (double)ulnow / (double)ultotal;
			}

			session->_progress.value_(progress);
		}
		return 0;
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
		__parse_http_header(header_string);
	}

	Headers::Headers(std::string& header_string)
	{
		__parse_http_header(header_string);
	}

	Headers::Headers(const std::initializer_list<Field>& headers)
	{
		for (const auto& header : headers)
		{
			_storage_headers[header.key_] = header.value_;
		}
	}

	curl_slist* Headers::Chunk()
	{
		struct curl_slist* chunk = nullptr;
		for (auto itr = _storage_headers.cbegin(); itr != _storage_headers.cend(); ++itr) 
		{
			auto header_string = std::string{ itr->first };
			if (itr->second.empty()) 
			{
				header_string += ";";
			}
			else 
			{
				header_string += ": " + itr->second;
			}
			chunk = curl_slist_append(chunk, header_string.data());
		}
		return chunk;
	}

	void Headers::__parse_http_header(std::string& header_string)
	{
		std::istringstream stream(header_string);
		{
			std::string line;
			while (std::getline(stream, line, '\n'))
			{
				if (line.length() > 0)
				{
					auto found = line.find(":");
					if (found != std::string::npos)
					{

						auto value = line.substr(found + 1);
						// erase space
						value.erase(0, value.find_first_not_of("\t "));
						value.resize(std::min(value.size(), value.find_last_not_of("\t\n\r ") + 1));

						_storage_headers[line.substr(0, found)] = value;

					}
				}
			}
		}
	}


	// ----------------------------------------------------------------------------------
	//
	//    Response 
	//
	// ----------------------------------------------------------------------------------

	Response::Response(int&& code, std::string&& body, Headers&& headers, std::string&& error)
		: code_(code), body_(body), headers_(headers), error_(error) {}


	// ----------------------------------------------------------------------------------
	//
	//    Multipart
	//
	// ----------------------------------------------------------------------------------

	Multipart::Multipart(const std::initializer_list<Part>& parts)
	{
		int index = 0;
		_parts.resize(parts.size());
		for (const auto& part : parts)
		{
			_parts[index++] = part;
		}
	}

	curl_mime* Multipart::Add2Curl(CURL *curl)
	{
		// todo 
		// subpart support
		if (curl && _parts.size() > 0)
		{
			curl_mime *mime = nullptr;
			mime = curl_mime_init(curl);
			curl_mimepart *mimepart;
			for (const auto& part : _parts)
			{
				if (part.is_file_)
				{
					mimepart = curl_mime_addpart(mime);
					curl_mime_filedata(mimepart, part.data_.c_str());
				}
				else
				{
					mimepart = curl_mime_addpart(mime);
					curl_mime_data(mimepart, part.data_.c_str(), CURL_ZERO_TERMINATED);
					curl_mime_type(mimepart, part.type_.c_str());
				}

				if (!part.name_.empty())
				{
					curl_mime_name(mimepart, part.name_.c_str());
				}

			}
			curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
			return mime;
		}
		return nullptr;
	}

}