// mime.h
#ifndef SIFD_MIME_H
#define SIFD_MIME_H

#include <string>
#include <map>

namespace sifd {

/**
 * MIME type detection system
 * 
 * Maps file extensions to HTTP Content-Type headers.
 * Designed for embedded systems with static lookup tables
 * rather than external configuration files.
 */
class MimeTypes {
public:
    /**
     * Get MIME type for a given file extension
     * @param extension File extension with dot (e.g., ".html")
     * @return MIME type string, defaults to "application/octet-stream"
     */
    static std::string get_type(const std::string& extension);

private:
    // Static MIME type lookup table
    static const std::map<std::string, std::string> mime_map_;
    
    // Default type for unknown extensions
    static const std::string DEFAULT_MIME;
};

} // namespace sifd

#endif // SIFD_MIME_H
