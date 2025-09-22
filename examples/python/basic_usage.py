# Demonstrates basic usage of the metadata library in Python.
#
# For this to work, you need the FRMetadta shared library created
# when BUILD_PYTHON_API=true somewhere in your python modules path.
# You can export the PYTHONPATH environment variable to point to the
# directory it's stored in if you've built it but haven't installed
# it yet

import FRMetadata

print("Creating a metadata object")
m = FRMetadata.Metadata()
print("Adding some trivial metadata")
m.add("Foo", "Bar", "Baz")
m.add("Foo", "wibble", "Wobble")
# Add a new ID and key through update
m.update("id", "another key", "another value")
print("IDs stored in metadata: ", m.ids())
print("Keys stored in Foo: ", m.keys("Foo"))
print("Keys stored in id: ", m.keys("id"))

print("Values stored in Foo:")
for key in m.keys("Foo"):
    print("Key = ", key, " Value = ", m.value("Foo", key))

print("Removing Foo['wibble']...")
m.erase("Foo", "wibble")
print("Keys stored in Foo: ", m.keys("Foo"))
print("Removing id metadata store completely...")
m.erase("id")
print("Checking to see if metadata still contains id: ", m.contains("id"))

print("Converting metadata to json...")
s = FRMetadata.Metadata.toJson(m)
print("JSON is: ", s)

print("Creating another metadata store from that json...")
# This is a bit awkward in python; you need to create an empty one
# and pass it to fromJson. It makes more sense in C++.
meatdata = FRMetadata.Metadata()
FRMetadata.Metadata.fromJson(meatdata, s)
print("IDs stored in meatdata: ", meatdata.ids())

# That's pretty much it!
