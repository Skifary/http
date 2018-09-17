# http

A lightweight http requests library.

### Example

```c++
// Get
auto resp = http::Get(
	http::URL{ "www.example.com" }
);
auto code = resp.code_; // http 200
auto body = resp.body_; // body, json or otherwise
auto error = resp.error_; // error
auto headers = resp.headers_; // response header
auto content_type = headers.GetField<std::string>("Content-Type"); // string field in header
auto content_length = headers.GetField<int>("Content-Length");  // int field in header

// GetAsync
http::GetAsync(
	[](http::Response resp) {
		// do something
	},
	http::URL{ "www.example.com" }
);
// All async method will return a std::future<void>
// if you want to intervene, use it.

// Parameters && Headers
auto resp = http::Get(
	http::URL{ "www.example.com" },
	http::Parameters{
		{ "username", "skifary" },
		{ "id", 10086 },
		{ "percent", 63.21 },
		{ "bool", true }
	},
	http::Headers{
		{ "Content-Type", "application/json" },
		{ "Content-Length", 351}
	}
);

// Download
// Set http::DownloadFilePath will write body into the given path.
// Set http::Progress will observer the progress.
auto resp = http::Get(
	http::URL{ "www.example.com" },
	http::DownloadFilePath {
		"local/path/file.c"
	},
	http::Progress{ [](double progress) {
			// progress
			std::cout << progress << std::endl;
		}
	}
);

// Upload
// Set http::Multipart for upload
// It supports both file path and memory
auto resp = http::Post(
	http::URL{ "www.example.com" },
	http::Multipart{
		{"local/path/file.c", /*part name*/"file"}, // from file path
		{"image/png", (http::byte_t *)"image_data" , /*part name*/"image"} // from memory
	}
);
```

Set options of interest into the http method's parameters.

The currently(2018-9-17) options include  `URL`  `Parameters`  `Headers`  `DownloadFilePath `  `Progress`  `Multipart`.

The currently(2018-9-17) methods include  `Get`  `Post`  `Head` and it's `async` version. 

More options and methods is on the way.

### Todo

* More HTTP method
* More options
* HTTPS

### Requirement

##### Required

* libcurl 7.56.0 or newer
* C++ 11 compiler

##### Optional

* Google Test

### Usage

##### Source code

* In this way, you should build `libcurl` for your own environment. For more please see [curl](https://github.com/curl/curl).
* Then drag the `http.h` and `http.cpp` into your project and build it.

##### Windows



##### Mac OS



### Special thanks

[libcurl](https://curl.haxx.se/libcurl/) the multiprotocol file transfer library

