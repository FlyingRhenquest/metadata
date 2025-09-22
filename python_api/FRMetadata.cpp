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
 *  Exposes an API to Metadata that can be accessed via Python.
 */


#include <fr/metadata/metadata.h>
#include <fr/metadata/server.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <memory>

using namespace fr::metadata;

NB_MODULE(FRMetadata, m) {

  // Python API for Metadata object
  
  nanobind::class_<Metadata>(m, "Metadata")
    // We want it to return a shared pointer so we can share it with C++ objects that use its resources
    .def(nanobind::new_([](){ return std::make_shared<Metadata>(); }))
    .def("contains", nanobind::overload_cast<const std::string&>(&Metadata::contains), "Returns true if metadata contains the specified ID or false if it does not. Each ID in a Metadata object will point to a separate key/value store.")
    .def("idContains", &Metadata::idContains, "Returns true if metadata stored in ID contains a key.")
    .def("add", nanobind::overload_cast<const std::string&>(&Metadata::add), "Add an empty metadata store with a specified ID.")
    .def("add", nanobind::overload_cast<const std::string&, const std::string&, const std::string&>(&Metadata::add), "Adds a key/value pair to a metadata store.")
    .def("ids", &Metadata::ids, "Returns all the IDs stored in this Metadata object")
    .def("keys", &Metadata::keys, "Returns all the keys in the metadata stored in the provided ID.")
    .def("value", &Metadata::value, "Returns the value stored in a key")
    .def("erase", nanobind::overload_cast<const std::string&>(&Metadata::erase), "Erases all the metadata stored in ID")
    .def("erase", nanobind::overload_cast<const std::string&, const std::string&>(&Metadata::erase), "Erases the provided key stored in the provided ID (call order is ID, key)")
    .def("update", &Metadata::update, "Update the value of a key in an ID. This will create the ID and the key if they don't exist, so you can use it to create them if you don't care if they already exist.")
    .def_static("toJson", &Metadata::toJson, "Convert a metadata to json. This is a static method and must be passed a metadata object")
    .def_static("fromJson", &Metadata::fromJson, "Populate a (presumably empty) metadata object from JSON. This is a static method and must be provided a Metadata object and the JSON string you want to populate it with.")
    ;

  // Python API for server object

  nanobind::class_<Server>(m, "Server")
    .def(nanobind::init<std::shared_ptr<Metadata>,int >())
    .def("start", &Server::start, "Starts server.")
    .def("shutdown", &Server::shutdown, "Shuts server down.")
    ;
  
}
