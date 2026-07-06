// sif.h
#ifndef SIFD_SIF_H
#define SIFD_SIF_H

#include <string>
#include <map>

namespace sifd {

/**
 * Samsung Interactive File (.sif) processor
 * 
 * Processes .sif files which are HTML-like documents with special tags.
 * Uses a simple text scanner approach rather than a full parser/lexer.
 * 
 * Supported tags:
 * - <sif echo="text">        : Output text
 * - <sif include="file">     : Include another file
 * - <sif read="path">        : Read and display file contents
 * - <sif get="param">        : Display GET parameter value
 * - <sif exec="program">     : Execute binary and display output
 */
class SifProcessor {
public:
    /**
     * Process a .sif file and generate HTML output
     * @param content Raw .sif file content
     * @param website_root Root directory of the website
     * @param get_params Map of GET parameters
     * @return Processed HTML output
     */
    static std::string process(const std::string& content,
                              const std::string& website_root,
                              const std::map<std::string, std::string>& get_params);

private:
    // Maximum include depth to prevent infinite recursion
    static constexpr int MAX_INCLUDE_DEPTH = 10;
    
    /**
     * Process a single SIF tag
     * @param tag_content Content inside <sif ...>
     * @param website_root Website root path
     * @param get_params GET parameters
     * @return Processed output for this tag
     */
    static std::string process_tag(const std::string& tag_content,
                                  const std::string& website_root,
                                  const std::map<std::string, std::string>& get_params);
    
    /**
     * Extract attribute value from tag content
     * Handles: attrib="value" or attrib='value'
     * @param content Tag content
     * @param attribute Attribute name to find
     * @return Attribute value or empty string
     */
    static std::string get_attribute(const std::string& content,
                                    const std::string& attribute);
    
    /**
     * Handle echo tag: <sif echo="text">
     */
    static std::string handle_echo(const std::string& content);
    
    /**
     * Handle include tag: <sif include="file.sif">
     */
    static std::string handle_include(const std::string& content,
                                     const std::string& website_root,
                                     const std::map<std::string, std::string>& get_params,
                                     int depth);
    
    /**
     * Handle read tag: <sif read="/proc/version">
     */
    static std::string handle_read(const std::string& content);
    
    /**
     * Handle get tag: <sif get="param_name">
     */
    static std::string handle_get(const std::string& content,
                                 const std::map<std::string, std::string>& get_params);
    
    /**
     * Handle exec tag: <sif exec="program">
     */
    static std::string handle_exec(const std::string& content,
                                  const std::string& website_root);
};

} // namespace sifd

#endif // SIFD_SIF_H
