# Readme
MissionPostProcessingTools provides a collection of utilities to help DCS mission makers work directly with .miz files created in the DCS mission editor where this is more convenient or adds options not available in the built-in mission editor. 

# MissionPostProcessingTool
Allows the mission file to be edited with a combination of Lua scripts and your favourite notepad or spreadsheet app. 

## Usage 
Simply drag and drop your .miz file onto `MissionPostProcessingTool.exe`. Your mission file data is converted to a csv file and this file is automatically opened in your default editor (or as configured). Make the desired changes to the csv, save the file and close the editor to apply your changes.

The effects of doing this depend on the lua scripts selected. The default script `main.lua` allows the configuration of radio presets for client airframes in the mission, grouped by country and airframe type. If more script options are available, you will be asked to select which to run.

Each time a .miz file is edited a backup will be created in the `backups` directory. Just in case your carefully created mission file gets corrupted during the process.

## Configuration

Use `config.lua` to override the csv editor to open, and possibly other options in future.

## Adding lua scripts

To add extra functionality, add lua files in the `input` directory, defining `applyCsv` and `extractCsv` following the example in `main.lua`.

# MissionPostProcessingTool_Kneeboard
Simplifies mission kneeboard management by automating packing and unpacking

## Usage 
Simply drag and drop your .miz file onto `MissionPostProcessingTool_Kneeboards.exe`. You will be prompted whether to clear existing kneeboards from the miz file before syncing up with a directory in the `input\kneeboards` folder:

* **Keep** Any kneeboard in the miz file and not in the unpacked directory will be unpacked and added.
* **Clear** Any kneeboard in the miz file and not in the unpacked directory will be removed from the mission.

In either case, `input\kneeboards\\<mission_filename>` will be synced up with the kneeboards in the mission file once the app runs successfully. Any files already in that directory will not be overwritten, so that changes to the unpacked images are applied to the mission file but not vice versa. 

**N.B.** if the version of a kneeboard in the .miz file is newer than the unpacked version changes will be lost!

Each time a .miz file is edited a backup will be created in the `backups` directory. Just in case your carefully created mission file gets corrupted during the process.

## Kneeboard files and directories

Kneeboard images should be placed in the appropriate `IMAGES` directory for the airframes they will be accessible to. This can be all airframes, or only particular types. DCS does not seem to support kneeboards per coalition or per country at the moment.

* `input\kneeboards\\<mission_filename>\\IMAGES` for kneeboards available to all players
* `input\kneeboards\\<mission_filename>\\<airframe_name>\\IMAGES` for kneeboards available to players in a specific airframe type

Kneeboard images typically have an aspect ratios of between `1.4` and `1.6`.

This tool will automatically create directories for all client airframes detected in the mission file.

## Configuration
Use `config_kneeboard.lua` to override the csv editor to open, and possibly other options in future.

# MissionPostProcessingToolMizDiff

Experimental app for testing/debugging the other tools. Pass it two .miz files to output 'diff.lua' to highlight what changed between the two missions.