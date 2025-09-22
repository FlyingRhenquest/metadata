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
 * This object takes a Pistache router reference and instruments
 * it to serve files in a specified directory from a specified route.
 * It will serve all the files in the directory from the route but
 * will not currently recurse into subdirectories.
 *
 * It will also optionally take a Metadata shared pointer, and if one
 * is passed, it will carve out a "routes" metadata entry and an entry
 * for the route in creates. This allows you to examine the routes
 * it creates using the webapp and is primarily intended for debugging.
 */

#pragma once

#include <filesystem>
#include <format>
#include <fr/metadata/magic_wrapper.h>
#include <fr/metadata/metadata.h>
#include <fr/metadata/server.h>
#include <memory>
#include <pistache/common.h>
#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>


namespace fr::metadata {
  
  /**
   * UiHelper instruments directories to be served from a pistache
   * route. This should make exposing the javascript UI pretty
   * straightforward.  Typically you wouldn't do this and would serve
   * them from apache or something. For this puposes of this demo I
   * don't want to require setting up an entire front-end environment
   * just to run this project.
   *
   * If you break this out and use it on its own, this object
   * should last the entire time that your server does, as it
   * holds the method to serve the files and passes itself to
   * pistache as the owner of that method.
   */
  
  class UiHelper {    
    Pistache::Rest::Router &router;
    std::shared_ptr<Metadata> data;
    MagicWrapper magic;
    
    void serveStaticFile(const Pistache::Rest::Request& request,
			 Pistache::Http::ResponseWriter response) {      
      // As currently designed, the path to this resource should only be one
      // directory deep. This should still work if I ever make it recursive,
      // but it depends on how the metadata is set up
      std::filesystem::path requestResource(request.resource());
      std::filesystem::path requestPath = requestResource.parent_path();
      // The absolute filename for this route should be accessible in
      // metadata now...
      std::string dataRoute(requestPath.string());
      dataRoute.erase(0,1);
      std::string absoluteFile = data->value(dataRoute, requestResource.string());
      Pistache::Http::serveFile(response, absoluteFile, Pistache::Http::Mime::MediaType::fromString(magic.mimeType(absoluteFile)));
    }
    
  public:
    UiHelper(Pistache::Rest::Router &router,
	     std::shared_ptr<Metadata> optionalMetadata = nullptr) : router(router), data(optionalMetadata) {
      if (!optionalMetadata) {
	// I AM going to use metadata to handle my routes, though.
	// If you pass it metadata from the serve, you'll be able
	// to see and change the routes in the metadata. This
	// is not really an intended feature, but might be interesting
	// to explore in the future. This would be very unsecure to
	// expose in a production environment, but does open some
	// possibilites for debugging and gathering metics.
	data = std::make_shared<Metadata>();
      }
    }
    ~UiHelper() = default;
    
    // Set up router to serve files in directory. Use absolute directory
    // location for best results.
    void instrumentDirectory(const std::string &directory, const std::string &pistacheRoute) {
      // Remove the initial / from the pistache route
      data->add("routes", pistacheRoute, directory);
      
      std::filesystem::path p(directory);
      
      for (const auto& entry : std::filesystem::directory_iterator(p)) {
	// Ignore file if it's a directory or something or starts with dot
	if (std::filesystem::is_regular_file(entry)
	    && entry.path().filename().string()[0] != '.') {
	  // Location of file in filesystem
	  std::string fileSystemLoc = entry.path().string();
	  // Location (route) to request file from Pistache
	  std::string pistacheLoc = std::format("{}/{}", pistacheRoute, entry.path().filename().string());
	  std::string dataRoute(pistacheRoute);
	  // Erase initial / for metadata entry
	  dataRoute.erase(0,1);
	  // Inscribe route metadata for this file	  
	  data->add(dataRoute, pistacheLoc, fileSystemLoc);

	  // Set up route callback
	  Pistache::Rest::Routes::Get(router, pistacheLoc,
				      Pistache::Rest::Routes::bind(&UiHelper::serveStaticFile, this));
	}
	
      }
    }
    
  };
  
}
