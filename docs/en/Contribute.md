# Technology selection
### Why Qt and not a game engine?
The main reason is: free game engines can provide support for a very tiny part of the project. Yes, sprites, effects, scene are handy; but if you look closely, the rendering code is 
just a small part of the whole logic. If I gave out 10% of the code to integrate with another framework, then more code would be spent on integrating Qt and another engine.
Further, the game is very closely tight dialogues and game content. Adventure map drawing is done in around 300 lines of code, but making sure that all the dialogues on the adventure map are still is much more code.
It is much easier and more pleasant to implement interactive elements with an engine that was created for GUI (Qt), than with a game engine.
In the end, the savings in efforts from using the GUI toolkit for most of the code greatly outweighs the possible inconveniences in low-level rendering of the scene.
In terms of sound and audio, yes, Qt is not the best; but for the sake of just one work with sound, I don't want to drag another addiction.

### Choice of Lua, Sol, RTTR and Nlohmann Json
The Lua + Sol bundle was chosen because Lua is extremely convenient to embed in a project; the language is also used iften in game development; 
Lua, unlike ChaiScript, can separate execution and compilation, if desired, and there is also the possibility of using JIT in the future - i.e. suitable for performance-critical sections somewhere in the template generator, for example. Sol is chosen just as the most popular C ++ binding for Lua.
RTTR was chosen as a non-invasive reflection library; most of it is used to (de)serialize all game objects with Nlohmann Json. The latter was chosen for its convenient syntax and pleasant debugging (speed not the top convern); also a plus went to the support for binary formats, which I may use for the network code.

### The project hardly uses C ++ 20, why then such a high requirement?
First, I plan to use more and more features of this standard. I don't want to limit myself right now. Secondly, it is very, very difficult to raise the support in the future. By the time the project matures, toolchains with the required level of support will be in distributions :)
In a nutshell, it's hard to raise the requirement later, and even harder to lower it (when a bunch of code is already using the new standard).

### Why C ++ and not Rust at all? Isn't it better to avoid outdated technology?
The language was chosen only on the basis of the maximum development speed. I have the most experience with C ++ (15 years). If I start learning Rust, I may not finish the project on it at all within the tight deadlines indicated for myself. In addition, a big obstacle is the fact that for the main GUI framework - Qt - there are no bindings to Rust (and indeed, Rust does not have too much choice with GUI toolkits yet).

# Design solutions

### Why are there so many raw pointers in the project? Have you heard of smart pointers?
The author has heard and used. Firstly, smart pointers are not a panacea for solving the problem of lifespan. When there is a game model where a bunch of objects can refer to each other - shared pointers are not an option due to circular references. Weak pointers can be used, but then you need to make a lock and check the result every time you access the object. Thus, there is no big difference in terms of lifetime violations. (yes, a radical difference - in case of violation of the logic of the program, in one case it will be nullptr, and in the other - a garbage pointer. This can be important). But I made a debatable choice that convenience when writing code, when referring to related objects, is much more important. Lifetime of library objects is quite definite and concentrated in one place (GameDatabase). New objects should not be created or destroyed during the game, the objects database becomes immutable.

### Why are there so many entities for the same thing? LibraryUnit, AdventureStack, BattleStack - and not withing inheritance chain - Is the author aware of inheritance?
Yes, from a design perspective, ideally an Adventure Mode creature is "a" library creature. So inheritance would be better ideologically. But there is one major drawback in C ++ - the constness within a method applies to all its fields at once. Those. if BattleStack inherited from AdventureStack and LibraryUnit, it could call non-const base methods. Yes, you can come up with a complex scheme where all non-constant getters are private, access through special helpers, fields are also hidden, everything is cool, but I think this is inconvenient and ugly. The advantages of the scheme, when each "inherited" object has a constant reference to the parent:
- firstly, saving memory, all fields of the parent are stored in one instance (and this is important, because it can often be so in the base data class much more)
- secondly, there is no need to come up with any schemes with getters. It's just that all fields are public and that's it. No side effects
- the schema is much more suitable for editing in the map editor - we can easily change the base class, leaving all the inherited custom parameters, no additional is needed. copying.
- there may be "empty objects" without a parent
- for serialization, we save only the id by the pointer to the base object, again, saving resources.
A similar scheme is used for GUI wrappers over standard classes.

### what is this Library / Adventure / Battle split?
Library - Database with parameters of game objects. "reference" values, characteristics that do not change during the game session.
Adventure - objects during the game in adventure mode are created when the game starts. Hero, castle, map object, guard, etc.
Battle - objects that live only for the duration of one battle. After the end of the battle, some of the data can be transferred to the adventure objects (for example, army losses or awards)

### Can you briefly describe the complete workflow of loading the game?
1. all index files of resources are loaded - text files, where the resource id and its location are described in a csv-like format.
2. based on the dependencies written in the index files, the loading order is determined. For example SoD => HotA, since HotA replaces some of the images.
3. index files are loaded and filled in maps for each resource type.
4. the list of resources of the "database" type is searched for.
5. for each database of the game, a list of json files that need to be loaded is calculated (based on dependencies)
6. for the database from the "databaseId" config, all json is loaded in turn into the GameDatabase.
7. Each json contains one or more sections (usually to an object of the same type, but not necessarily), and in turn, each subsequent one can overwrite or override the data of the previous ones. All records are loaded into the database.
8. during loading, all references (constant pointers more precisely) in objects to each other are initialized. in the case of broken identifiers, an exception will be thrown (thus, all non-optional links are guaranteed to be live).
9. Localization files are loaded (qt- from Qt resources, as well as localization for database objects - from resource indices.)
10. Gui wrappers are created over all library objects that allow getting localized strings, images and sound effects.
After that, the engine can be conditionally considered ready to work. You can create Adventure * objects and pass anything to them.


### Where is the logic of the game? Calculation of damage, skills, effect from artifacts?
For this, GameLogic implements the BattleEstimation and AdventureEstimation classes. They control the computation of derived values ​​- the effect of spells, skills, abilities, and so on.
They fill in the Estimated fields in Battle and Adventure objects.
With damage, the truth is not so simple, it is already considered inside the class that is actually responsible for conducting battles - BattleManager. I would not say that this is a well-designed piece of code, rather the opposite, but I try to take it from it periodically to various other places.

### Why not spread the Adventure / Battle reflection across different dll / shared?
For reflection, there is a certain limitation that it lives inside one binary module. It can be exported, but this is quite inconvenient and expensive, unfortunately. Because for serialization, you may need objects of a different level, while you have to keep them all in one library (albeit separated by files).

### Why in Adventure objects some of the logic is done inline methods, and some free functions in Estimation?
In fact, I would like them to have no methods at all (other than a constructor). Still, for small short getters, sometimes you want to make a break. I'm not sure how good it is with the definition of "what is short and small" so far.

### What is this DependencyInjector?
This can be said to be an experiment, which I  am trying to get rid of. The main motivation is that in regular C ++, hard dependencies usually go as parameters to the constructor. Or, the constructor is passed as a parameter of DependencyResolver from some DI framework. But in the forms that Qt and uic generate, the only option for the class is to have one QWidget parameter, a pointer to the parent. As a result, for objects placed on the form, additional dependencies, models have to be set separately in some init(...) or setModels(...) method. It turns out that part of the logic moves there. You can get around this by pulling dependencies from the property of the parent widget. Then you can have dependencies already as reference fields, for example. Unfortunately, this scheme has one bold minus - everything is rather unclear and hidden. Plus, the object that we created can pull out a dependency that we might not want to open to it.
In general, I initially relied on this mechanism as an alternative to global variables, but I am gradually moving away from it and reducing the number of such dependencies.

### GoldenStyle - what kind of horror is this? My eyes are bleeding
Yes. Sorry.

### Feeling like the BattleEmulator class is kind of bold with huge methods
This class is something like a sandbox from which the code is periodically dispersed to different places. Another thing is that there are other classes that would also be useful for cutting - BattleWidget, BattleFieldItem, BattleManager. Something is cut out of them, but not as quickly as I would like.

### You invented a profiler class, your own logger, and not ashamed?
You know, no. As long as they are small and do not require extraordinary I am ok with accusations in the NIH-syndrome. The logger and profiler is the one that, if necessary, is corrected by search and replacement.

### Are there any non-awful files to look at?
IGameDatabase.hpp, GraphicsLibrary.cpp, LibraryReflection.cpp, LibraryWrappers.hpp. Not perfect, but I don't seem to be ashamed.