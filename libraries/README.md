The headers and the .so for the GVR NDK live inside the sdk-base.aar. Before
compiling an NDK sample, the necessary code needs to be extracted from the aar
into the /libraries directory. Depending on the build system used by your app
you can copy this code or manually extract the headers and native libs.

See the root build.gradle file for an example extractNdk task.
