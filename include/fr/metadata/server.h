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
 * A Rest server for a Metadata object. Uses pistache.
 */

#pragma once

#include <atomic>
#include <filesystem>
#include <format>
#include <mutex>
#include <thread>
#include <fr/metadata/metadata.h>
#include <fr/metadata/ui_helper.h>
#include <pistache/common.h>
#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>

namespace fr::metadata {

  /**
   * Server sets up Pistache to server a metadata object via a REST interface.
   * It is created with a shared pointer that can be passed in from C++ or
   * python. Calling start() will start the server up, and it will run
   * until you call shutdown() on it.
   *
   * This is designed to be a trivial service used to illustrate the
   * capabilities provided if you take the time to set up multiple
   * interfaces to a C++ object. If you want to use it for production
   * work, you'll want to know a bit more about Pistache than I do
   * at the moment.
   */
  class Server {
    // Metdata object to provide data to the REST API
    std::shared_ptr<Metadata> data;
    // The actual Pistache server that handles the requests
    Pistache::Http::Endpoint server;
    // The Pistache rotuer that has all my REST routes defined
    Pistache::Rest::Router router;
    std::shared_ptr<UiHelper> helper;
    std::atomic<bool> shutdownFlag;
    std::atomic<bool> running;
    std::thread serverThread;
#ifdef EXPOSE_UI
    constexpr static bool expose_ui = true;
#else
    constexpr static bool expose_ui = false;
#endif
    
    void error(Pistache::Http::ResponseWriter& response, const std::string& wat, Pistache::Http::Code code = Pistache::Http::Code::Bad_Request) {
      response.send(code, wat);
    }

    // Returns when the route /metadata is called.
    void allIdsHandler(const Pistache::Rest::Request& request,
		       Pistache::Http::ResponseWriter response) {
      auto contentType = request.headers().tryGet<Pistache::Http::Header::ContentType>();
      auto stream = response.stream(Pistache::Http::Code::Ok);
      
      std::string message;
      for(const auto& id : data->ids()) {
	  message = std::format("<a href=\"http://127.0.0.1:8080/metadata/{}\">{}</a><br/>", id, id);
	  stream.write(message.c_str(), message.length());
      }
      stream << Pistache::Http::ends;
    }
    
    // Add a new ID in response to a post
    void addId(const Pistache::Rest::Request &request,
	       Pistache::Http::ResponseWriter response) {
      auto id = request.param(":id").as<std::string>();
      if (data->contains(id)) {
	error(response, "ID already exists");
      } else {
	try {
	  data->add(id);
	  response.send(Pistache::Http::Code::Ok, "ID Added\n");
	} catch(std::exception &e) {
	  error(response, e.what());
	}
      }
    }

    void getId(const Pistache::Rest::Request& request,
	       Pistache::Http::ResponseWriter response) {
      auto id = request.param(":id").as<std::string>();
      if (!data->contains(id)) {
	std::string err = std::format("'{}' not found", id);
	error(response, err, Pistache::Http::Code::Not_Found);
      } else {
	auto stream = response.stream(Pistache::Http::Code::Ok);
	for (const auto &key : data->keys(id)) {
	  const std::string message = std::format("{} = {}\n", key, data->value(id, key));
	  stream.write(message.c_str(), message.length());
	}
	stream << Pistache::Http::ends;
      }
    }

    void uiTopLevel(const Pistache::Rest::Request& request,
		    Pistache::Http::ResponseWriter response) {
      // Expect ui directory to be in current directory
      std::string requestPath = request.resource();
      std::filesystem::path p = std::filesystem::current_path();
      std::string theFile = p.string();
      theFile.append(requestPath);
      if (std::filesystem::exists(theFile)) {
	const char *mimetype;
	magic_t magic;
	magic = magic_open(MAGIC_MIME_TYPE | MAGIC_MIME_ENCODING);
	magic_load(magic, "/usr/share/misc/magic.mgc");
	mimetype = magic_file(magic, theFile.c_str());
	std::cout << "Serving " << theFile << " mime-type: " << mimetype << std::endl;
	Pistache::Http::serveFile(response, theFile, Pistache::Http::Mime::MediaType::fromString(mimetype));
	magic_close(magic);
      }      
    }

    void setupRoutes() {
      running = false;
      Pistache::Rest::Routes::Get(router, "/metadata",
				  Pistache::Rest::Routes::bind(&Server::allIdsHandler, this));
      Pistache::Rest::Routes::Post(router, "/metadata/:id",
				   Pistache::Rest::Routes::bind(&Server::addId, this));
      Pistache::Rest::Routes::Get(router, "/metadata/:id",
				  Pistache::Rest::Routes::bind(&Server::getId, this));


      // Set up routes to expose UI. React seems to want the various directories under "dist" set up as
      // individual routes, so I'll just instrument them all individually using a helper class.
      if constexpr(expose_ui) {
	helper = std::make_shared<UiHelper>(router, data);
	
	std::filesystem::path p = std::filesystem::current_path() / "ui/dist";
	// current working directory
	std::string cwd = p.string();
	// Instrument dist directory. I'm going to remap the name to "ui", which
	// I think is a bit more descriptive. index.html should be the only thing in there anyway.
	helper->instrumentDirectory(std::filesystem::absolute(p).string(), "/ui");
	
	for (const auto& entry : std::filesystem::recursive_directory_iterator(p)) {
	  if (std::filesystem::is_directory(entry)) {
	    // This should give me the last directory name in the path
	    std::string pistacheRoute = std::format("/{}", entry.path().filename().string());
	    std::string absolutePath = std::filesystem::absolute(entry).string();
	    std::cout << "Setting up route for " << pistacheRoute << "," << absolutePath << std::endl;
	    helper->instrumentDirectory(absolutePath, pistacheRoute);
	  }
	  
	}
      } 
    }
    
  public:
    
    Server(std::shared_ptr<Metadata> meatdata, int port) : data(meatdata), server({Pistache::Ipv4::any(), Pistache::Port(port)}) {
      setupRoutes();
    }
    
    Server(std::shared_ptr<Metadata> meatdata, const Pistache::Address &address) : data(meatdata), server(address) {
      setupRoutes();
    }

    ~Server() {
      if (running) {
	shutdown();
      }
    }
    
    void start(int nthreads = 1) {

      // Serve blocks, so we need to kick off a thread for this to
      // not block

      if (!running) {
	serverThread = std::thread([this, nthreads]() {
	  running = true;
	  shutdownFlag = false;
	  auto opts = Pistache::Http::Endpoint::options().threads(nthreads);
	  server.init(opts);
	  server.setHandler(router.handler());
	  server.serve();	  
	});
      } else {
	throw(std::runtime_error("Server is already running."));
      }
    }
    
    void shutdown() {
      if (running) {
	shutdownFlag = true;
	server.shutdown();
	serverThread.join();
	running = false;
      }
    }
  };
  
}
