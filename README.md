# Project Progression

**Ethan Schledewitz-Edwards**

**Lucas O'Leary**


## Group Formation
When forming our group, we decided to stick to working with other members of our GDW group. This would allow us to contribute to our GDW game, resulting in the time spent working on this project contributing to both this class and the game, rather than having to spend more time working on two separate projects. Working with people from the same GDW team also means that we are already familiar with each other’s strengths and weaknesses, and can efficiently divide up the workload accordingly, as opposed to trying to adapt on the fly.

### Our Projected Timeline
<img width="3049" height="1195" alt="Screenshot 2025-10-21 213027" src="https://github.com/user-attachments/assets/73762ebd-3733-4889-91d1-05fd0ffe4a04" />


## Interactive Media Scenario
Our interactive media scenario is an extension of our GDW game demo level. This level will utilize all the core mechanics within the game, with an emphasis on the implementations made for this project. As the game is a 3D puzzle platformer that takes place in a grandfather clock, the level will include various clock-related hazards, such as moving platforms, gears, and pitfalls. The level is divided up into smaller sections which focus on a specific mechanic to get through. The level will include UI elements, to showcase the UI-focused implementations, as well as core gameplay elements such as checkpoints and prop spawners to further highlight the implementations made, such as factory and observer. Finally, there will be a toggleable developer console that utilizes commands to trigger events in the scene through text, such as loading a specific checkpoint, or changing game variables, allowing for quicker playtesting and bug fixing.

## Singleton
Our project has implemented the singleton design pattern in two forms: a Game Manager, and a UI Manager.

As with most games, making a game manager was the clear immediate first choice, as it gives a single, central script to handle the overall state of the game. In our scenario, the game manager handles the location of the player’s spawn points as they progress through the level. When the player reaches a checkpoint, the blueprint calls for the spawn point within the game manager to be updated, which in turn allows the respawn function within the player health script to get the correct spawn position from the game manager. 

```mermaid
flowchart TD
    A[GameManager]:::manager -->|Stores & Updates| B[Spawn Point]
    B --> C[Player Spawn Location]

    D[BP_Checkpoint]:::checkpoint -->|Player Location| A

    F[Player Death]:::death -->|Triggers Respawn Function| E[PlayerHealth]
    E -->|Requests Spawn Point| A
    A -->|Provides Correct Spawn| E
    E -->|Respawns Player At| C

    classDef manager fill:#4a90e2,stroke:#2c3e50,stroke-width:2px,color:#fff
    classDef checkpoint fill:#f39c12,stroke:#d35400,stroke-width:2px,color:#fff
    classDef death fill:#e74c3c,stroke:#922b21,stroke-width:2px,color:#fff
```
Having it centralized this way means that there are far less variables to keep track of as opposed to each script containing duplicate variables, or making multiple calls to other scripts to get commonly used values, leading to “spaghetti code”. This streamlines development by keeping the codebase streamlined, comprehensive, and predictable.

## Factory
In terms of the factory design pattern, our project has two implementations in the form of a zone UI generator, which can dynamically create zone-dependent UI elements, and a “prop dropper”, which can spawn various props in our game by dropping them in from above.

The zone UI factory implementation leverages Unreal Engine’s User Widgets to generate UI elements on the screen, regardless of the specifics of the UI. This works by having a widget blueprint for a given UI layer, such as HUD, which then has a stack that stores all the UI elements in that layer. When a player enters a new area, it runs a function in that area’s blueprint - BP_LevelArea - that clears all the elements of the stack in its layer. It then passes its UI class to the UI factory method, which then creates a widget based on the class passed to it, and outputs it onto the screen. This allows for easy expansion in future development, by allowing new zone UI to be created with a single function call, regardless of the specifics of that element. This greatly streamlines the level creation process, reduces redundant UI type checks in the code, and removes the headache of creating new zone UI types in the future.

```mermaid
flowchart TD
    A[Widget Blueprint]:::widget -->|Contains| B[UI Stack]
    B -->|Stores| C[UI Elements]

    D[Player Enters New Area]:::event --> E[BP_LevelArea]
    E -->|Clears| B
    E -->|Passes UI Class| F[UI Factory Method]
    F -->|Creates Widget From Class| G[User Widget]
    G -->|Outputs To| H[Screen]:::screen

    classDef widget fill:#4a90e2,stroke:#2c3e50,stroke-width:2px,color:#fff
    classDef event fill:#27ae60,stroke:#145a32,stroke-width:2px,color:#fff
    classDef screen fill:#6bc9ff,stroke:#4785a8,stroke-width:2px,color:#fff
```
The second factory implementation, the “prop dropper”, is used to dynamically spawn a given prop in a specific position. In terms of gameplay, this could be something like spawning a gear overhead upon entering a level, which then falls to the ground, and is then used to solve a puzzle. This implementation works by using the blueprint “BP_DynamicObjectSpawner”, which takes the specified transform and object class, and spawns in an actor of that class at the transform. This means that any prop actor class, such as a screw or gear, can be spawned at a specified transform, without needing actor-specific spawn scripts. This, much like the other implementation, means that future development, especially level design, will be more streamlined, and not require the creation of new spawners for every new prop type created.

```mermaid
flowchart TD
    A[Spawn Event Triggered]:::event --> B[BP_DynamicObjectSpawner]:::spawner
    B -->|Takes Transform + Class| C[SpawnActor]
    C --> D[Prop Appears]
    D -->|Falls To Ground| E[Used in Gameplay]

    classDef event fill:#27ae60,stroke:#145a32,stroke-width:2px,color:#fff
    classDef spawner fill:#4a90e2,stroke:#2c3e50,stroke-width:2px,color:#fff
```
