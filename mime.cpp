// mime.cpp
#include "mime.h"

namespace sifd {

const std::string MimeTypes::DEFAULT_MIME = "application/octet-stream";

// Initialize static MIME type map
// This covers all types specified in requirements plus common variants
const std::map<std::string, std::string> MimeTypes::mime_map_ = {
    // Web content
    {".html", "text/html; charset=utf-8"},
    {".htm", "text/html; charset=utf-8"},
    {".sif", "text/html; charset=utf-8"},     // SIF files are HTML-like
    {".css", "text/css; charset=utf-8"},
    {".js", "application/javascript; charset=utf-8"},
    {".json", "application/json; charset=utf-8"},
    {".xml", "application/xml; charset=utf-8"},
    {".txt", "text/plain; charset=utf-8"},
    
    // Images
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".webp", "image/webp"},
    {".svg", "image/svg+xml"},
    {".ico", "image/x-icon"},
    {".bmp", "image/bmp"},
    
    // Fonts
    {".ttf", "font/ttf"},
    {".otf", "font/otf"},
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    
    // Media
    {".mp4", "video/mp4"},
    {".mp3", "audio/mpeg"},
    {".webm", "video/webm"},
    {".ogg", "audio/ogg"},
    
    // Documents
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".tar", "application/x-tar"},
    {".gz", "application/gzip"}
};

std::string MimeTypes::get_type(const std::string& extension) {
    auto it = mime_map_.find(extension);
    if (it != mime_map_.end()) {
        return it->second;
    }
    return DEFAULT_MIME;
}

} // namespace sifd
