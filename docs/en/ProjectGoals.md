# Problems the project is trying to solve
1. Lack of a free engine that is being developed and used for a large group of players on a daily basis. There is VCMI, but it is stable, not under active development.
2. Development of HotA lags behind the expectations of the community:  
2.1. The pace of development for the HotA add-on is much lower than expected: releases are essentially once a year.  
2.2. Proprietary development of both HotA and HD Mod. The development is completely closed. Lack of public "test" builds of HotA. If the project is closed, no one will be able to "pick up" it and continue what has been gained over the years.  
2.3. The inability of the community to directly influence development in the form of fixing bugs or implementing features.  
3. HD Mod - no alternatives  
3.1. If the lobby breaks down, no one can take and switch to an alternative server, or raise their own  
3.2. Player's rating is bound to one lobby server.  
4. SoD and HotA are incompatible projects. No one can use plugins for SoD, because HotA itself is like a very heavy plugin. As a result, we have Fresh Mod being developed; but again, there is no way to fully mix balanced and gameplay features of SoD / HotA. For example, play with Cove faction, but on SoD rules. There are also a lot of people who want to play custom SoD like Paragon, but with HotA UI features (MP and exchange between heroes in the castle). This is also partially solved by Fresh Mod, and this is again a diversification of development.
5. Lack of features typical for modern e-sports games: replays, observer mode, matchmaking.


# Goals and objectives of the project
Development of a free analogue of HotA + HD Mod, as a program (excluding game resources). It is necessary that anyone can build (and modify) the game engine and the server side for it.
The main task is to develop an open implementation of features for Heroes as an eSports discipline (observer mode, replays, matchmaking, etc.). At the same time, the engine should, just like the original, support singleplayer in offline mode - both on random and fixed maps.
1. Openness and decentralization. Despite the presence of some official builds or lobby servers, anyone should be able to establish the same infrastructure.
2. Public development process. Bug tracker, feature requests, release and update planning - everything should be completely open and transparent.
3. Respect for any state of the game in terms of balance. Anyone should be able to play (with certain settings), "the same SoD", "HOTA 1.5.0", etc. A game, while providing new features and balance changes, should always provide full forward compatibility with older releases. Those. the conditional FreeHeroes 1.1 assembly is released with balance changes, everyone can continue to play (and rated games) and version 1.0.
4. DO NOT attempt to resolve the copyright issue of the NWC / Ubisoft game content. Just like the VCMI project, it will completely depend on the SoD / HotA preinstalled on the computer. The project will not focus on creating analogs / replayers of current resources (only to the extent necessary "the game starts and does not crash").
5. Cross-platform; for both client and server side. Win / Mac / Linux, mobile platforms (Android) in perspective.
6. Lay the potential for using different AI engines (including using Machine Learning).
7. The project should not be presented in any way as a replacement or competitor to HotA. Anti-PR or belittling "related" projects - HotA, HD Mod, VCMI are not allowed.

# Why not VCMI?
The author spent several months trying to do a little refactoring of the graphical part of the VCMI. Unfortunately, the quality of the code is such that it is not possible to make a change without touching and rewriting almost everything. As a result, I started to create a separate graphical client within the VCMI project. After some time, I realized that due to the coherence of the code, I have to edit almost everything. It turned out that I was writing with VCMI almost from scratch, and I would have to test every component - but there were no tests for all my edits.
Ideologically, I would say in spirit that FreeHeroes is like “VCMI 2.0”, under a different license and with changed priorities (an online game is at the head).

One huge difference point between VCMI and FreeHeroes - is license, VCMI is using GPL, while FreeHeroes is using MIT. So closed project on top of VCMI is impossible.


# Brief summary of key features

An incomplete list of features that are considered key for the future project, in comparison with HotA + HDMod and VCMI.
1. Support for network play, conceptually protected from most cheats. A maphack should be impossible simply because the client will not have the necessary information.
2. As a tool # 1 for analysis both for the player and for the developer, it becomes a replay (both for the whole game as a whole, and a replay for one battle). Replay serves well for fighting cheaters too.
3. Observer / streamer mode for playing
4. The mode of simultaneous turns, which allows simultaneous actions for the team without interrupting the ST (as well as the possibility of "endless ST" until the end of the game)
5. Possibility of flexible switching between SoD / HotA rules, adding features and balance changes not in bulk, but partially. The release of new FreeHeroes versions balance should allow players to play any previously released version of the balance and rules.
6. The server part for searching for opponents (lobby) should be as decentralized as possible; in case of project closure / domain expiration / loss of interest of the FreeHeroes author, any enthusiast should promptly raise a replacement while maintaining the user base with their rating. The rating server must be separate from the game server.
7. Refine and develop artificial intelligence, provide researchers to create their own versions of AI, which can fight both among themselves and with humans. Bots should have the same rights as humans!
8. Compatibility with HotA, at least one-sided. There must be an import of templates, as well as import of single maps and HotA campaigns; priority on template import compatibility. Also, as new versions of HotA are released, all features should be quickly brought into the engine.
9. There should be no “abuses” of the game’s flaws, such as “timer abuse” (additional waiting when the timer is stopped on some dialogue).
10. It should be convenient, easy and pleasant to create tournaments and show matches with custom templates and rules for modders, streamers and players alike.

# History of past versions and RoadMap for future releases
(plan) 0.4 - Adventure mode
- Map editor
- Import of SoD / HotA cards
- Minimal functionality of random maps
- implementation of most of the objects of the adventure map
- the ability to play over the network 1: 1.

(plan) 0.3 - completion of work on battles:
- Complete all creature abilities;
- Combat vehicles, city siege;
- Rebuilding the city, buying an army and heroes;
- Dialogue exchange between 2 heroes.
- (maximum plan: lay the foundation for some form of network mode).

0.2, 08/12/2020 - more than half of the battle-related functionality implemented. Major features:
- ** ! ** Editing hero parameters in the configurator, skills, artifacts;
- ** ! ** Hero dialogue - control of a doll with artifacts + control of army stacks;
- ** ! ** Artificial Intelligence and Autoboy (at the basic level)
- ** ! ** Ability to record and replay battle replays;
- ** ! ** Implemented a spell book, dialogue for it and the ability to conjure magic of two types - buffs / debuffs and shock spells.
- Artifacts - Done 122/154
- Spells - 35/71
- Secondary skills - 19/29
- Hero specializations - 37/63
- Creature abilities - 33/118

0.1, 03.07.2020 - the first test release.
- ** ! ** HotA Resource Conversion Utility
- ** ! ** Test window BattleEmulator - the ability to select the army of each side and its location.
- ** ! ** Battle window
- ** ! ** Settings window - sound, animation speed
- Animations of creatures in battle (except for ability animations)
- Action in battle - wait, defend, move, melee and ranged attack
- Calculation of damage and retaliatory damage
- Action log
- Some abilities: flight, teleport, double attack, unanswered attack, 2 and endless responses.
- Accounting for penalties: close combat for shooters, distance
- Calculating accessibility for wide creatures. Additional direction of attack for wide creatures.
  
0.0, 06/01/2020 - start of work on the project.