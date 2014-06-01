#include "file_type.h"

std::string file_type(const std::string &name)
{
	static const std::unordered_map<std::string, std::string> file_type_data_ = {
		{ "txt",	"text/plain"	 },
		{ "html",	"text/html"		 },
		{ "xml",	"text/xml"		 },
		{ "js",		"text/javascript"},
		{ "css",	"text/css"		 },
		
		{ "gif",	"image/gif"		 },
		{ "jpg",	"image/jpeg"	 },
		{ "png",	"image/png"		 },
		
		{ "mp3",	"audio/mpeg"	 },
		{ "mp4",	"video/mp4"		 }
	};
	
	if (name.empty()) return "other";
	auto it = file_type_data_.find(name);
	if (it == file_type_data_.end()) return "other";
	return it->second;
}
