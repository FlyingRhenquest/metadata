# Metadata Demo

This is a trivial full-stack project intended to demonstrate a few
handy C++ things. I'm using it to learn my way around Pistache,
nanobind (a pybind11 successor) and eventually React.

What you'll find here:

 * C++ objects, mainly metadata.h and server.h in include. Metadata allows you to create key/value data stores indexed by a top-level ID. You access different key/value stores based on the top level ID. It provides a simple UI to make accessing IDs, keys and values straightfoward. Server uses pistache to provide a REST interface and serve the React front-end.
 * Python API for C++ objects -- create a Metadata and a Server in Python and interact with them in the Python memory space.
 * A simple React webapp that is barely more than the stock one created with Vite. This is enough to demonstrate that serving the React UI from Pistache works, and that it can access the REST data provided by the backend.
 
# Requirements

 * Naonobind for the C++ Python API
 * Python and Python C dev libraries for Nanobind.
 * Pistache to provide REST services from C++
 * libmagic-dev to identify MIME types of files being served
 * React/Vite for the front end. The npm-install-ui CMake target builds the react front end to the CMake current binary directory.
 
# Building

I'm doing this in Linux, but it should work in Windows or OSX as well,
as long as you have all the things installed where CMake can find
them.

   mkdir /tmp/build
   cmake ~/sandbox/metadata
   make
   
If it builds OK, the next step is the fun bit.

   python3
   
   import FRMetadata
   
   m = FRMetadata.Metadata()
   
This creates a C++ shared pointer and returns the object to Python. So you can


   help(m)
   
   
If you want to. At this point m.ids() will return an empty array
because you haven't installed any IDs in m yet. If you create a server
with this Metadata object, the server will install some IDs and keys
to serve up the REST API:

  s = FRMetadata.Server(m, 8080)
  
If you try m.ids() now, you'll see "assets, "routes" and "ui". You can
try m.keys("assets") to list the keys stored under the "assets" ID.

Start the server with the number of threads you want it to run:

   s.start(1)

Now fire up a browser and navigate to http://127.0.0.1:8080/metadata
and you should be able to see the assets, routes and ui entries. You
can get and post to this interface with curl or something. If you add
or remove entries from m in python, it will be reflected on the web
page. Posting or removing keys via the REST interface will be
reflected in the m object in Python.

Check the react UI at http://127.0.0.1:8080/ui/index.html. All it does
is load in the HTML from the top level REST API, so it's not that
interesting, but this does demonstrate everything you need to serve
the React UI from Pistache.

You can s.shutdown() when you're done or just exit() out of python.

That's pretty much all I had planned for this simple demo, as I didn't
want a lot of extraneous stuff to get in the way of what I was trying
to learn. I'll probably do some more with the React UI in the future,
as I do want to spend some time learning my way around that.

Hope you find it useful!
