These are c++ recreations of the audio systems in a project I worked on
The original systems were entirely blueprints, and these are c++ recreations

Note that this is not the entire UE project, this is just the Source folder of code files

The biggest system is the dialogue system, which has pretty extensive queueing, sequencing, interrupting, delays, timeouts, validity-checks for lines of dialogue that can be triggered during gameplay of a game

The main idea is that "line_001.wav" can be 'attempted' at multiple stages during a level, and based on the data settings, it could fail to play due to be being on timeout, or due to only being allowed to play once in the level, etc.

All dialogue data is housed in a FDialogueLine struct.

Dialogue Sequences are arrays of FDialogueLines that queue together, and if they become invalid, they get removed as a group
