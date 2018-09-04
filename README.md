# Hammer2K13 Project

[![Build status](https://ci.appveyor.com/api/projects/status/paaregcki0gur8r3?svg=true)](https://ci.appveyor.com/project/Gocnak/sdk-2013-hammer)

Hammer2K13 is the Hammer project from the 2007 Source Engine leak brought to the Source SDK 2013 platform by SCell555. It aims to bring Hammer back to parity with the SDK 2013 hammer, and add features such as no vertex precision loss (as found in Hammerpatch by Crashfort), raise the quality of life and workflow of using Hammer (removing defunct/useless features such as Carve and Goldsource support) and fix up the various things broken with it (Lighting Preview, general crashes, etc).

My personal goals for this project is to make it lean, and focused on Source map making (specifically for [Momentum Mod](https://github.com/momentum-mod/game)). This means getting rid of all goldsource support, and broken features such as Carve. Eventual goals can be tied in, such as moving over to Qt to potentially make Hammer2K13 cross-platform, and GPU accelerated VRAD.

## TODO/Progress

Speaking of aspirations and goals, you may track progress of the project through the [Projects](https://github.com/Gocnak/sdk-2013-hammer/projects/1) page. Features can be requested, and bugs can be reported inside of the [Issues](https://github.com/Gocnak/sdk-2013-hammer/issues) section.

## Prebuilt Releases

You can find the latest pre-built release [on AppVeyor](https://ci.appveyor.com/project/Gocnak/sdk-2013-hammer/build/artifacts). Just extract the contents of the zip into your game's root bin/ directory (where Hammer is typically located).

## Building the Project

The project builds on Visual Studio 2017. But since Hammer is as old as your father, it uses the MFC/ATL libraries, which you need to ***ensure you have, through the Visual Studio Installer!***

![EasyScreen](https://i.imgur.com/hzchUOF.png)

![Individual](https://i.imgur.com/YMz9wpB.png)

Simply: 

1. Clone this project
2. Run the `createhammerproject.bat` file  
*Note: If you get an error about "Unable to find RegKey for .vcproj files in solutions", follow [this fix](https://developer.valvesoftware.com/wiki/Source_SDK_2013#Unable_to_find_RegKey_for_.vcproj_files_in_solutions_.28Windows.29)*
3. Open `hammer.sln` and compile
4. Copy the contents of the bin/ folder (minus the pbd files, unless you really like them) into your game's root bin/ folder (where Hammer is located)

## License

Hammer belongs to Valve. I'm just sad it's not open source, because this sort of thing could have happened much sooner. All rights reserved to Valve.