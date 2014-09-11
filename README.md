Open Limit Adjuster
==============================

This is a open source limit adjuster for Grand Theft Auto III, Vice City and San Andreas aiming to bring back limit adjusters to the scene.

However it does not aim to be just a limit ***adjuster***, but a limit ***purger***, that's, turning whatever previously was limited into unlimited, being only limited by the machine/application capacity.

Not sure how to contribute? Take a look [here](https://guides.github.com/activities/contributing-to-open-source/) and [here](https://github.com/ThirteenAG/limit_adjuster_gta3vcsa/blob/master/doc/Creating%20Your%20Own%20Adjuster.md).
Not sure what to contribute? Check the [open issues](https://github.com/ThirteenAG/limit_adjuster_gta3vcsa/issues).

### Goals
 * Bring limit adjusters back to the scene, but not totally centralized into one person
 * Avoid little limit adjusters here and there
 * Be an extremely stable limit adjuster
 * Purge out limits from the face of the community, we are in a dynamic world!

### Compiling

The source code targets pre-C++11 compilers with naked attribute, that is Visual Studio 2010 and above.

You'll need to following tools to compile the source code.
+ [CMake](http://www.cmake.org/) 2.8 or greater
+ [Visual Studio](http://www.visualstudio.com/downloads) 2010 or greater

Then run CMake in the base source directory to generate the projects file in the *msvc/* directory.
By using command line, this can be done by:

    mkdir msvc
    cd msvc
    cmake ../

That's it, you have a Visual Studio solution now, you are ready to contribute and compile.
