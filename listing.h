// listing.h
#ifndef SIFD_LISTING_H
#define SIFD_LISTING_H

#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

namespace sifd {

/**
 * Directory listing generator
 * 
 * Creates Apache-style HTML directory listings when no index file exists.
 * Features:
 * - Directories listed first
 * - Alphabetical sorting
 * - File size display
 * - Modification time display
 * - Proper HTML escaping
 */
class DirectoryListing {
public:
    /**
     * Generate HTML directory listing
     * @param dir_path Absolute filesystem path
     * @param url_path URL path for links
     * @return Complete HTML page with directory listing
     */
    static std::string generate(const std::string& dir_path, 
                               const std::string& url_path);

private:
    struct DirEntry {
        std::string name;
        bool is_dir;
        off_t size;
        time_t mtime;
    };
    
    static bool compare_entries(const DirEntry& a, const DirEntry& b);
    static std::string format_size(off_t size);
    static std::string format_time(time_t t);
    static std::string html_encode(const std::string& text);
};

} // namespace sifd

#endif // SIFD_LISTING_H
