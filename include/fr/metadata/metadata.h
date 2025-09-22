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
 * This is a object used to store metadata. It is fairly trivial, basically
 * a std::map of keys, which are unique identifiers of the metadata being
 * stored, and a shared pointer to a map<string,string> of key/value pairs.
 * This does not allow you to store arbitrary objects as metadata, but
 * you can still store a lot of useful stuff.
 *
 * This object provides serialization/deserialziation via cereal and
 * is able to use the json, XML or binary archivers.
 */

#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <format>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace fr::metadata {

  // Metadata provides thread-safe metadata lookup
  // Metadata stores a map of unique string IDs and each
  // ID provides access to a map of string key/value pairs.
  // You can add or remove key/value pairs or entire IDs
  // via the Metadata API.
  
  class Metadata {
  public:
    // Set up some type names
    
    // Define storage for the actual key/value pairs
    using DataType = std::map<std::string,std::string>;
    using Data = std::shared_ptr<DataType>;
    // Define storage for the metadata itself. The first
    // element must be a unique identifier of some sort (A
    // UUID or sha5sum would be good for larger scale things.)
    using MetadataMap = std::map<std::string, Data>;

  private:

    MetadataMap metadata;
    std::mutex mtx;

    // Checks to see if an ID exists in metadata. You can
    // override lock if you know you don't need to be thread safe.
    bool contains(const std::string& id, bool lock) {
      bool retval = false;
      if (lock) {
	std::lock_guard<std::mutex> lock(mtx);
	retval = metadata.contains(id);
      } else {
	retval = metadata.contains(id);
      }
      return retval;
    }
    
  public:

    Metadata() = default;
    ~Metadata() = default;

    
    // Forward some map calls on to metadata and the various
    // metadata maps contained by metadata

    bool contains(const std::string& id) {
      return(contains(id, true));
    }

    // Checks to see if a key exists in the map contained in
    // ID. Also returns false if ID does not exist.

    bool idContains(const std::string& id, const std::string& key) {
      std::lock_guard<std::mutex> lock(mtx);
      bool retval = false;
      retval = contains(id, false) && metadata.at(id)->contains(key);
      return retval;
    }

    // Create an empty metadata store at an ID
    void add(const std::string& id) {
      std::lock_guard<std::mutex> lock(mtx);
      if (contains(id, false)) {
	std::string errstr = std::format("'{}' already exists in metadata", id);
	throw std::runtime_error(errstr);
      }
      metadata.insert({id, std::make_shared<DataType>()});
    }

    // Create a key/value pair in a metadata store.

    void add(const std::string& id, const std::string& key,
	     const std::string& value) {
      if (idContains(id, key)) {
	std::string errstr = std::format("'{}' already exists in the unique id '{}'", key, id);
	throw std::runtime_error(errstr);
      } else {
	// We need to add the ID if it's not already in there. Unfortunately
	// this locks again. This case should happen relatively
	// infrequently (citation neeed) so I'll not worry too much
	// about optimizing it right now. I can add another special case
	// to idContains, but I really don't want to. I could also try to
	// come up with a more generic locking solution using lambdas
	// or something, but I also really don't want to. And I probably
	// at some point should set up a templated locking strategy
	// object so we can create one of these that never locks for
	// non-threaded solutions, but that'd distract me from the
	// basic gist of my demo right now.
	if (!contains(id)) {
	  add(id);
	}
      }
      std::lock_guard<std::mutex> lock(mtx);
      try {
	const auto [itr, success] = metadata.at(id)->insert({key, value});
	if (!success) {
	  // This can only happen if key already existed in the store
	  std::string errstr = std::format("Key '{}' already exists in the unique id '{}'", key, id);
	  throw std::runtime_error(errstr);
	}
      } catch (std::exception& e) {
	// This can only happen if id does not exist
	std::string errstr = std::format("Unique ID '{}' does not exist", id);
	throw std::runtime_error(errstr);
      }
    }

    // Returns a vector of strings containing all the IDs currently
    // in metadata. Yes I return this by copy. Yes I'm OK with that.

    std::vector<std::string> ids() {
      std::vector<std::string> allIds;
      std::lock_guard<std::mutex> lock(mtx);
      for (auto itr = metadata.begin(); itr != metadata.end(); ++itr) {
	allIds.push_back(itr->first);
      }
      return allIds;
    }

    // Returns a vector of strings containing the keys in the
    // id metadata store.
    std::vector<std::string> keys(const std::string& id) {
      std::vector<std::string> allKeys;
      std::lock_guard<std::mutex> lock(mtx);
      if (!contains(id, false)) {
	std::string errstr = std::format("Unique ID '{}' does not exist", id);
	throw std::runtime_error(errstr);	
      }
      for (auto itr = metadata.at(id)->begin();
	   itr != metadata.at(id)->end(); ++itr) {
	allKeys.push_back(itr->first);
      }
      return allKeys;
    }

    // Returns the string value stored at id,key
    std::string value(const std::string& id, const std::string& key) {
      std::string retval;
      if (!idContains(id, key)) {
	std::string errstr = std::format("Key '{}' or unique ID '{}' do not exist", key, id);
	throw std::runtime_error(errstr);
      }
      std::lock_guard<std::mutex> lock(mtx);
      try {
	retval = metadata.at(id)->at(key);
      } catch (std::exception& e) {
	// I'm not even sure I could make it happen to test
	// if you ever actually get this exception
	std::string errstr = std::format("ID '{}' or key '{}' was deleted before I could access it (This is probably highly unusual but technically possible.)", id, key);
	throw(errstr);
      }
      return retval;
    }

    // Erase an entire ID
    void erase(const std::string& id) {
      std::lock_guard<std::mutex> lock(mtx);
      metadata.erase(id);
    }

    // Erase a key in an ID
    void erase(const std::string& id, const std::string& key) {
      std::lock_guard<std::mutex> lock(mtx);
      if (contains(id, false)) {
	metadata.at(id)->erase(key);
      }
    }

    // Update a key in an ID. This will create the key if it doesn't
    // already exist, so it can also be used as a no-throw create
    // if you want to use it that way.
    void update(const std::string& id, const std::string &key, const std::string& value) {
      if (!contains(id)) {
	add(id);
      }
      std::lock_guard<std::mutex> lock(mtx);
      (*metadata.at(id))[key]=value;
    }

    // Cereal archiver
    template <class Archive>
    void serialize(Archive& archive) {
      archive(metadata);
    }

    // Convert a Metadata to JSON (Using the cereal archiver)
    static std::string toJson(Metadata& m) {
      std::stringstream stream;
      {
	cereal::JSONOutputArchive archive(stream);
	archive(CEREAL_NVP(m));
      }
      return stream.str();
    }

    // Convert a JSON string to a Metadata. This actually
    // populates a (presumably empty) metadata you provide
    static void fromJson(Metadata& m, const std::string& data) {
      std::stringstream stream;
      stream << data;
      {
	cereal::JSONInputArchive archive(stream);
	std::lock_guard<std::mutex> lock(m.mtx);
	archive(m);
      }
    }
    
  };
  
}
