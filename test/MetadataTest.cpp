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
 * Tests for the metadata object
 */

#include <gtest/gtest.h>
#include <fr/metadata/metadata.h>
#include <iostream>
#include <sstream>

using namespace fr::metadata;

TEST(Metadata, BasicFunctionality) {
  Metadata m;
  // Check for key  
  ASSERT_FALSE(m.contains("Foo"));
  ASSERT_FALSE(m.idContains("Foo", "Bar"));
  // Add the key and check again
  m.add("Foo");
  ASSERT_TRUE(m.contains("Foo"));
  ASSERT_FALSE(m.idContains("Foo", "Bar"));
  m.add("Foo", "Bar", "Baz");
  // Check ids() functionality
  auto ids = m.ids();
  ASSERT_EQ(ids.size(), 1);
  ASSERT_EQ(ids[0], "Foo");
  ASSERT_EQ(m.value("Foo", "Bar"), "Baz");
  // Create a new metadata store and store something in it in one call
  m.add("Baz", "Quux", "Florble");
  ASSERT_EQ(m.value("Baz", "Quux"), "Florble");
  ASSERT_EQ(m.ids().size(), 2);
  // Check keys() functionality
  auto keys = m.keys("Foo");
  ASSERT_EQ(keys.size(), 1); // Only one key in "foo" right now
  ASSERT_EQ(keys[0], "Bar");
  // Let's add another one
  m.add("Foo", "Pleh", "value");
  keys = m.keys("Foo");
  ASSERT_EQ(keys.size(), 2);
  // Change bar to something else
  m.update("Foo", "Bar", "Florble");
  ASSERT_EQ(m.value("Foo", "Bar"), "Florble");
  m.erase("Foo", "Bar");
  ASSERT_FALSE(m.idContains("Foo", "Bar"));
  keys = m.keys("Foo");
  ASSERT_EQ(keys[0], "Pleh");
  // Use Update to add an ID/Key
  m.update("id", "ego", "superego");
  ASSERT_EQ(m.value("id", "ego"), "superego");
  // Delete all the metadata held in Foo
  m.erase("Foo");
  ASSERT_FALSE(m.contains("Foo"));
}

// Verify trying to overwrite keys fails
TEST(Metadata, FailCases) {
  Metadata m;
  bool exceptionCaught = false;
  m.add("Foo");
  try {
    m.add("Foo");
  } catch (std::exception &e) {
    exceptionCaught = true;
  }
  ASSERT_TRUE(exceptionCaught);
  exceptionCaught = false;
  m.add("Bar", "Baz", "Quux");
  try {
    m.add("Bar", "Baz", "Quux");
  } catch (std::exception &e) {
    exceptionCaught = true;    
  }
  ASSERT_TRUE(exceptionCaught);
}

TEST(Metadata, Serialization) {
  Metadata m;
  m.add("Foo", "Bar", "Baz");
  m.add("Foo", "Bait", "Quux");
  // Serialize to a StringStream (JSON)

  // Cereal's serializer doesn't flush the stream until it's destroyed,
  // and the easiest way to do that is to make another scope
  std::stringstream stream;
  {
    cereal::JSONOutputArchive archive(stream);
    archive(CEREAL_NVP(m));
  }
  std::cout << "Metadata JSON Data: " << stream.str() << std::endl;
  // Deserialize
  Metadata meatdata;
  // Best to do this in its own scope too
  {
    cereal::JSONInputArchive archive(stream);
    // If you use CEREAL_NVP on deserialization, Cereal will enforce the
    // variable name the JSON was encoded with. So if you're serializing
    // m and then deserialize to m later, that should work, but you'll
    // get a runtime error if you try to deserialize meatdata from m
    // that way.
    archive(meatdata);
  }
  ASSERT_EQ(m.value("Foo", "Bar"), meatdata.value("Foo", "Bar"));
  ASSERT_EQ(m.value("Foo", "Bait"), meatdata.value("Foo", "Bait"));
}

// Test toJson functionality. This isn't a very good test,
// it'll just verify you get a non-zero-length string back
TEST(Metadata, tojson) {
  Metadata m;
  m.update("Foo", "Bar", "Baz");
  std::string json = Metadata::toJson(m);
  std::cout << "Json Data: " << json << std::endl;
  ASSERT_GT(json.length(), 0);
}

// Test fromJson functionality
TEST(Metadata, fromjson) {
  Metadata m;
  m.update("Foo", "Bar", "Baz");
  std::string json = Metadata::toJson(m);
  Metadata meatdata;
  Metadata::fromJson(meatdata, json);
  ASSERT_EQ(m.value("Foo", "Bar"), meatdata.value("Foo", "Bar"));
}
