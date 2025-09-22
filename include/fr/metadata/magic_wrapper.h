/**
 * Copyright 2025 Bruce Ide
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * A RAII wrapper for libmagic. This holds a magic handle and uses it to
 * look up mime types for files.
 *
 * This isn't intended to be used with the Python API, but if anyone would
 * find it useful to expose it, it wouldn't be terriby difficult. That should
 * probably go in a PR to the magic maintainers along with some of the additional
 * functionality libmagic provides that I'm not using, which might be worth
 * spinning a different project up for.
 */

#pragma once

#include <magic.h>
#include <format>
#include <stdexcept>
#include <string>

namespace fr::metadata {

  /**
   * This is a RAII wrapper for the libmagic functionality I need.
   * (It basically just guesses mime types.)
   */
  
  class MagicWrapper {
    // This location is fairly standard on most Linux boxes.
    static inline constexpr std::string_view defaultMagicFile = "/usr/share/misc/magic.mgc" ;
    // Always return this is opening magic failed. The UI will probably not
    // behave correctly, though, so the exceptions should not just be ignored.
    static inline constexpr std::string_view mimeTypeUnknown = "application/octect-stream";
    magic_t magic;
  public:
    MagicWrapper(std::string file = "") : magic(nullptr) {
      if (file.empty()) {
	file = std::string(defaultMagicFile);
      }
      magic = magic_open(MAGIC_MIME);
      if (nullptr == magic) {
	throw std::runtime_error("Unable to open libmagic. Mime types will not be reported correctly.");
      }
      if (0 != magic_load(magic, file.c_str())) {
	std::string wat = std::format("Unable to load magic file '{}'. Mime types will not be reported correctly.", file);
	throw std::runtime_error(wat);
      }      
    }

    ~MagicWrapper() {
      if (magic) {
	magic_close(magic);
      }
    }

    // Tries to figure out the mime type of the filename and return it as
    // a string.
    std::string mimeType(const std::string& filename) {
      const char *mimetype = nullptr;
      if (!magic) {
	// No magic for you!
	return std::string(mimeTypeUnknown);
      }
      mimetype = magic_file(magic, filename.c_str());
      if (nullptr == mimetype) {
	return std::string(mimeTypeUnknown);
      }
      return std::string(mimetype);
    }
    
  };
  
}


