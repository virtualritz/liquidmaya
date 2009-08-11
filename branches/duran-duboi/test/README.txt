
Liquid Test Suite
-----------------

This directory is intended for the development of a set of scenes and scripts for 
semi-automatic regression testing of Liquid.

It is currently empty, I'm just commiting this directory as a reminder to get started on this...

The general idea is to have a series of scenes, each one thoroughly showing all permutations of a 
single aspect of Liquid's translation, e.g. a subdiv scene, a nurbs scene, a particles scene, etc.
Then we can render each scene, save the resulting RIB or image as a benchmark, and have scripts to
automatically re-render and compare against the benchmarks in future releases, warning the user
when it finds differences.

--
Andrew Chapman - March 2003
