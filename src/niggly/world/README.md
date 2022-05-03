
 * Account
   - id, name, password, access-level, list of users
 * User
   - associated account, id, name
 * Scene
 * Entity (Node on a graph)
   - Type (Region, Zone, User, NPC, etc.)
   - Name+Id
   - Description
   - Attributes - must support quantity... how much butter
   - Conditions - changing a condition can activate logic
   - Contained entities (eg., a chit can have a marker on it)
   - Portals (to allow transfer for contained entities to other entities)
   - Collection of Logic modules
   - Collection of Command modules (commands that can be given)
 * Regions isa Entity (Map, or anything that can contain entities)
   - collection of Zones (Entities that are nodes on a graph)
   - Logic for enter/exit
 * Portal (Edge on a graph between Entities)
   - enter/exit logic
   - associated region+zone
 * Action, something that attempts to mutate the state machine, could be a Timer
 * Logic, something that reacts to Actions (modifying/cancelling them)
 * Command, something that a User/NPC can do.
 * A command executes in a "transaction" that atomically mutates the state machine

----
The closure of implementations:
 * RFTS, impl1, 18xx, FieldMarshall, Shanghai Trader, Julius Caesar
----

4xStars:
 * Scenes:
   + Loading
   + Setup 
   + EnterUserMove (per-user)
     - MoveShips
     - EnterOrders
   + ReconcileTurn
   + StarBattle
   + AttackPlanet
   + Invasion
   + EndGame
 * Entity:
   + Player/NPC
   + StarSystem
   + Planet
   + Fleet
   + Ship (Transport, MarkI-IV)
   + Map (hex)
   + Debris
   
Impl1
 * Scenes
   + Loading
   + Setup
   + GlobalMap (per-user)
   + Trading
   + Diplomacy
   + Battle-Setup
   + Battle
 * Entities
   + Player/NPC
   + Different civilians
   + Different military/naval units
   + Region
   + BattleMap
   + BattleMapHex
   + BattleMapUnit

FieldMarshal
 * Scenes
   + Loading
   + Setup
   + 
