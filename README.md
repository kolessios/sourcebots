# SourceBots

SourceBots is an AI system to create Bots (CPU controlled players) for the Source Engine in a similar way to the NPC's AI. 

SouceBots can be implemented in:

* [Source SDK 2013](https://github.com/ValveSoftware/source-sdk-2013)
* [Alien Swarm branch](https://github.com/Sandern/aswscratch)

> ⚠️ Although the code works, this code is part of the [first C++ project](https://github.com/kolessios/insource-legacy) of the developer and it has several optimization problems. Not suitable for production use.

> ⚠️ This project is not actively under development but PR's are welcome.

> 🌎 Most of the comments and debug messages are in Spanish.

## 🍕 Features

- Create Bots and program their AI with a code similar to the [NPCs AI](https://developer.valvesoftware.com/wiki/AI_Programming_Overview).
  - [🔗](https://github.com/kolessios/sourcebots/blob/master/bots/schedules) Each set of tasks is separated into "schedules".
  - [🔗](https://github.com/kolessios/sourcebots/blob/master/bots/schedules/bot_schedule_hide_and_reload.cpp#L21) Each schedule executes [tasks](https://github.com/kolessios/sourcebots/blob/master/bots/bot_defs.h#L433) (actions to be performed by the Bot) from top to bottom.
  - [🔗](https://github.com/kolessios/sourcebots/blob/master/bots/schedules/bot_schedule_hide_and_reload.cpp#L34) Each schedule has interrupt conditions, the [conditions](https://github.com/kolessios/sourcebots/blob/master/bots/bot_defs.h#L597) are obtained in each frame according to the Bot status.
  - [🔗](https://github.com/kolessios/sourcebots/blob/master/bots/schedules/bot_schedule_hide_and_reload.cpp#L48) Each schedule has a level of desire. The Bot will start the schedule with the highest desire and will try to finish it (if no interruption condition occurs).
  - [🔗](https://github.com/kolessios/sourcebots/blob/master/bots/schedules/bot_schedule_hide_and_reload.cpp#L66) If necessary, you can add or modify the operation of the tasks in each schedule.
- [🔗](https://github.com/kolessios/sourcebots/tree/master/bots/components) Movement, vision, memory and others are componentized. You can add or remove capabilities according to the type of Bot you want to create.
- [🔗](https://github.com/kolessios/sourcebots/blob/master/bots/components/bot_component_locomotion.cpp#L111) Move to entities or vectors using the [Navigation mesh](https://developer.valvesoftware.com/wiki/Navigation_Meshes).
- Aim to entities or vectors.
- Detection of friends, enemies, neutral targets and objects such as weapons.
- [🔗](https://github.com/kolessios/sourcebots/blob/master/bots/components/bot_component_decision.cpp#L746) Ability to prioritize enemies.
- [🔗](https://github.com/kolessios/sourcebots/blob/master/bots/squad_manager.cpp) Create squads and make decisions respecitively.
- [🔗](https://github.com/kolessios/sourcebots/blob/master/bots/bot_utils.cpp#L114) Hitbox detection and customization of the "preferred" hitbox when aiming.
- [🔗](https://github.com/kolessios/sourcebots/blob/master/bots/components/bot_component_memory.cpp) Memory system to store information such as strings or numbers as well as positions of visible entities, including enemies and allies. 
- [🔗](https://github.com/kolessios/sourcebots/blob/master/bots/bot_skill.cpp) Easy configuration depending on the difficulty of the Bot.
- [🔗](https://github.com/kolessios/sourcebots/blob/master/bots/bot_maker.cpp) Map entities to create Bots in events, highly customizable.

## 📖 How to start

The [wiki](https://github.com/kolessios/sourcebots/wiki) has some help documents including installation and initial configuration.

## 🧪 TODO

You can find all comments with the word [TODO](https://github.com/kolessios/sourcebots/search?q=TODO&type=Code), try to fix as many as you can!

Some of them:

- Optimization problems.
- Detect and fix memory leaks.
- Navigation problems.
- Melee weapons
- Doors
- Make the code easier to read.
- Add English comments.
- Being able to use the AI in NPC's [(Like NextBot)](https://developer.valvesoftware.com/wiki/NextBot)

## 🎬 Videos

You can watch this [YouTube playlist](https://youtu.be/W5N_w7dwxuw?list=PLOUVJcNedgYEfzMJvK8wiI9GzvLKRR2IW) to see examples of Bots and a bit of the history of the project's development.

## 📞 Contact

You can contact me at:

* [@kolessios](https://twitter.com/kolessios)
* [Steam](http://steamcommunity.com/profiles/76561198040059089)
* kolessios [at] gmail.com
