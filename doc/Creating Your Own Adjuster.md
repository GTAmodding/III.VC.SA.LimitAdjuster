Creating Your Own Adjuster
=============================

This Limit Adjuster is very flexible when we talk about adding newer limits, it's pretty straightforward if you already know where to patch, you just need to create one cpp file with a class instance and that's it.

Creating The Adjuster Instance
--------------------------------

First of all you need a new cpp file to implement your adjuster class.
In our example we're going to call it *foo.cpp*, right, create it at the *src/limits* directory in a directory that fits it's category or none if unsure.

Then, you need to create a derived class from either `SimpleAdjuster` or  `Adjuster`, those are in the *LimitAdjuster.h* header.
The difference between then, is that `Adjuster` is more complex than `SimpleAdjuster` and is supposed to be used when one class will handle more than one limit. Generally you would use `SimpleAdjuster`.
You can check examples of adjusters at the **src/sample/** directory.

The interface is simple and event-based:

* **The Limit Name**
    +  `GetLimitName` in  `SimpleAdjuster` should return the name of the limit (for the ini file) or null if the limit is not supported at this time (i.e. because game is not supported).
    + `GetLimits` in `Adjuster` should return an static and null terminated array of `Limit` objects, each describes one limit, with name and identifier (possibily an user data value for latter use). It can also return null in case of incompatible game
* **Changing the Limit**
    + When the ini entry for the limit is found, `ChangeLimit` is called with the id of the limit and it's value string, here the limit should be patched
    + `ChangeLimit` is guaranteed to not get called more than once for each limit name.
    + The  `Adjuster::IsUnlimited ` should be used if it's necessary to check if the string has the *unlimited* value.
* **Pos Processing**
    + Optionally `Process` can be implemented to post process the values received at `ChangeLimit`, useful for complex adjusters with many limits where all of them are dependent on each other.
* **Telling the Limit Usage**
    + Optionally `GetUsage` can be implemented to tell the user how much of the limit has been used
    + The `Adjuster::GetUsage<T>` method can be used to straightly get the string from the usage and max usage integers.
* **Injector and Game Version Manager**
    + The basic frontend for patches is the injector library.
    + It includes a game version manager, that can be accessed from `Adjuster::GetGVM` or the shortcuts `Adjuster::IsIII`, `Adjuster::IsVC` and `Adjuster::IsSA`
    + You are not totally required to use the injector library if you are implementing a pre-existing adjuster, but making a biding for the other library that translates into injector calls is recommended.

Finally, you should instance the final adjuster object in the global scope, that will tell the Limit Adjuster about it's existence.
Don't forget to add the limit to the ini file at `doc/limit_adjuster_gta3vcsa.ini`.

If still in doubt, the best place to learn the interface are the in the already implemented adjusters itself. But as noted before simple samples are available at the *src/sample/* directory.
